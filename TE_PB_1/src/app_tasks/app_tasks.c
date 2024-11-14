/**
 * @file app_tasks.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 1.0
 * @date 2023-04-15
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "app_tasks.h"

#include "app_lora.h"
#include "app_settings.h"
#include "app_communication.h"
#include "app_vibration_analysis.h"
#include "app_nfc.h"
#include "app_saadc.h"
#include "app.h"

// Task specific defines
#define TASK_ADVERTISE_PERIOD          5     // sec
#define TASK_LED_PERIOD                30    // sec
#define VIBRATION_DATA_COUNT           10    // Number of vibration data to record before sending it.
#define TASK_ANALYSIS_SEND_PERIOD      3600  // sec
#define TASK_VIBRATION_ANALYSIS_PERIOD TASK_ANALYSIS_SEND_PERIOD / (float)VIBRATION_DATA_COUNT // sec  
#define LORA_FFT_PACKET_COUNT          58    // Number of FFT bins to send in each FFT packet.
#define BLE_FFT_PACKET_SIZE            32    // Bytes

// RTC Defines (Scheduler)
#define SEC_TO_TICK(period) period / WAKE_UP_TIME_SEC 

static uint16_t session_to_download = 0;
static bool is_advertising = false;

static void data_gathering(uint16_t record_duration);
static void task_perform_fft(void);
static void task_vibration_analysis(void);
static void task_advertise(void);
static void task_flash_led(void);
static void task_stop_advertising(void);
static void task_send_heart_beat(void);
static void task_send_data(void);
static void task_save_config(void);
static void task_download_data_ble(void);
static void task_download_fft_ble(void);
static void task_get_session_count(void);
static void task_get_fft_count(void);
static void task_erase_all(void);
static void task_erase_data(void);
static void task_erase_fft(void);
static void task_power_off_device(void);
static void task_data_request(void);

void app_task_set_advertising(bool val)
{
  is_advertising = val;
}

bool app_task_is_advertising(void)
{
  return is_advertising;
}

static void task_send_heart_beat(void)
{ 
  NRF_LOG_INFO("### SENDING HEART BEAT ###");
  if (app_lora_joined()){
    // Getting current device configuration.
    struct app_config_t config = {0};
    app_settings_get_configuration((uint8_t *)&config);
    
    // Prepare the packet to be send by LoRa.
    uint8_t data[sizeof(uint8_t) + sizeof(struct app_config_t) + sizeof(uint8_t)] = {0};

    // Getting flash usage percentage. 
    // This will get the number of block (sectors) used by littlefs divided by the number of block allowed by littlfs.
    app_flash_enable();
    data[0] = app_flash_get_percentage();
    app_flash_disable();

    // Writing the config in the buffer.
    memcpy(&data[1], &config, sizeof(struct app_config_t));

    // Getting battery voltage.
    nrf_saadc_value_t battery = 0;
    app_saadc_init();
    app_saadc_get_channel(3, &battery);
    app_saadc_uninit();
    
    float battery_v = ((battery * 0.6f) / 4096.0f) * 6.0f;
    uint8_t battery_data = (uint8_t)(battery_v * 10);

    NRF_LOG_INFO("Battery ADC: %d", battery);
    NRF_LOG_INFO("Battery voltage: %d", battery_data);

    data[17] = battery_data;
    
    // Send heart beat.
    app_lora_wakeup();
    app_lora_send_heartbeat(data, sizeof(uint8_t) + sizeof(struct app_config_t) + sizeof(uint8_t), true);
    app_lora_sleep();
  }
  app_hdw_wdt_kick();
  app_hdw_gpio_low_power(); // Sleep
}

static void task_perform_fft(void)
{
  NRF_LOG_INFO("### FFT ###");
  uint16_t mean_fft_buffer[FFT_SIZE/2] = {0}; // FFT_COMPUTE_LEN/2 because only need the Real side of the fft

  app_flash_enable(); // Enabling NOR flash and littlefs.
  // Remove fft_data file.
  app_flash_remove_fft_data();
  // Perform fft.
  app_vibration_fft(mean_fft_buffer, FFT_SIZE / 2);

  // Store the FFT in fft folder.
  // This also get the current fft id.
  uint16_t fft_id = 0;
  struct app_fft_header_t header = { // Header of the FFT.
    .fft_id = 0,
    .gain = fft_gain,
    .freq = fft_freq
  };

  if(!app_flash_create_fft_session(&fft_id, header)){ // Create a new session of fft.
    NRF_LOG_ERROR("Error creating fft session.");
  }
  NRF_LOG_INFO("FFT id: %d", fft_id);
  
  // Write the 4096 bytes FFT result in the file.
  uint8_t buffer[FFT_FLASH_WRITE_SIZE] = {0};
  for (uint16_t i = 0; i < FFT_SIZE / FFT_FLASH_WRITE_SIZE; ++i){
    memcpy(buffer, (uint8_t *)&mean_fft_buffer[i * (FFT_FLASH_WRITE_SIZE / 2)], FFT_FLASH_WRITE_SIZE);
    app_flash_record_fft_packet(buffer, FFT_FLASH_WRITE_SIZE);
  }

  app_flash_close_fft_session(); // Closing the FFT session.
  app_flash_disable(); // Disabling flash and unmount littlefs.
  
  // The FFT will be sent by LoRa only if BLE user is disconnected.
  // This way, the user can download the FFTs directly after to be able to show them.
  if(!ble_user_connected){ 
    if (app_lora_joined()){
      // Send FFT by LoRa.
      app_lora_wakeup();
    
      // Buffer for FFT packet
      uint16_t data[LORA_FFT_PACKET_COUNT] = {0};

      // The FFT result is sent by 116 bytes + 6 bytes overhead chunks (122 bytes total). 
      // The whole FFT is sent using 18 chunks.
      uint32_t data_cursor = 0;
      uint8_t chunk_id = 0;
      while(data_cursor < FFT_SIZE/2){
        for(uint8_t i = 0; i < LORA_FFT_PACKET_COUNT; ++i){
          if(data_cursor < FFT_SIZE/2){
            data[i] = mean_fft_buffer[data_cursor];
            data_cursor++;
          }
          else{
            data[i] = 0;
          }
        }
        NRF_LOG_INFO("Sending chunk %d", chunk_id);
        app_lora_send_fft_pkt(fft_id, chunk_id, fft_gain, fft_freq, (uint8_t *)data, 2 * LORA_FFT_PACKET_COUNT, false);
        chunk_id++;
        app_hdw_wdt_kick();
      }
      app_lora_sleep();
    }
  }
  app_hdw_wdt_kick();
  app_comm_send_response();
  app_hdw_gpio_low_power();
}

static void task_vibration_analysis(void)
{
  NRF_LOG_INFO("### VIBRATION ANALYSIS TASK ###");
  // Getting accel vibration RMS.
  uint16_t accel_mod = app_vibration_analyze();
  NRF_LOG_INFO("Accel module RMS: %d", accel_mod);

  // Storing vibration data in file.
  app_flash_enable();
  app_flash_append_vibration_data(accel_mod);

  // If vibration data filled with 10 measures, send the data by LoRa.
  uint32_t file_size = app_flash_get_vibration_data_size();
  NRF_LOG_INFO("Vibration data file size: %d", file_size);
  if(file_size >= VIBRATION_DATA_COUNT * sizeof(uint16_t)){
    // Send the data by LoRa.
    if(app_lora_joined()){
      // Prepare data packet.
      uint8_t data[sizeof(uint16_t) * VIBRATION_DATA_COUNT] = {0};
      app_flash_get_vibration_data(data, sizeof(uint16_t) * 10);

      app_lora_wakeup();
      app_lora_send_vibration_data_pkt(TASK_VIBRATION_ANALYSIS_PERIOD, data, sizeof(uint16_t) * VIBRATION_DATA_COUNT, true);
      app_lora_sleep();
    }

    // Remove the vibration data file.
    app_flash_remove_vibration_data();
  }

  app_flash_disable();

  app_hdw_wdt_kick();
  app_hdw_gpio_low_power();
}

static void data_gathering(uint16_t record_duration)
{
  struct app_packet_t sensor_data = {0};

  app_flash_enable(); // Enabling NOR flash and littlfs.
  uint16_t session_id = 0;
  if(!app_flash_create_data_session(&session_id)){ // Create a new session of data.
    NRF_LOG_ERROR("Error creating data session.");
  }
  NRF_LOG_INFO("Session id: %d", session_id);
  // Recording data. This will be performed even if there was an error with flash initialization
  // or session creation. The data will be sent by LoRa anyway. In case of a fail, the data 
  // will still be sent but with a record id of 0.
  app_peripherals_get_data(&sensor_data, session_id, record_duration); 
  app_flash_close_data_session(); // Closing data session.
  app_flash_disable(); // Disable NOR flash and littlefs.
  
  // Sending result by LoRa.
  if (app_lora_joined()){
    uint8_t data[sizeof(struct app_packet_t)] = {0};
    memcpy(&data[0], &sensor_data, sizeof(struct app_packet_t));

    app_lora_wakeup();
    app_lora_send_data_pkt(data, sizeof(struct app_packet_t), true);
    app_lora_sleep();
  }

  app_hdw_wdt_kick();
  app_hdw_gpio_low_power();
}

static void task_send_data(void)
{    
  NRF_LOG_INFO("### DATA GATHERING TASK ###");
  data_gathering(app_settings_get_record_duration_seconds());    
}

static void task_save_config(void)
{
  NRF_LOG_INFO("### SAVE CONFIG TASK ###");
  bool need_reset = false;
            
  // Disable soft device
  if (nrf_sdh_is_enabled()){ // When changing config with BLE.
    APP_ERROR_CHECK(nrf_sdh_disable_request());
    need_reset = true;
  }
  app_flash_save_config();
  
  NRF_LOG_INFO("The device will reset.");
  nrf_delay_ms(100);

  if (need_reset){ // When changing config with BLE. Reset.
    NVIC_SystemReset();
  }
  
  SCH_Modify_Task(task_send_heart_beat, 0, SEC_TO_TICK(app_settings_get_lora_heartbeat_period_minutes() * 60), true); // Reset the heartbeat task.
  SCH_Modify_Task(task_send_data, SEC_TO_TICK(app_settings_get_record_period_minutes() * 60), SEC_TO_TICK(app_settings_get_record_period_minutes() * 60), false); // Reset the data record task.
  // If this config is set at 0, don't start automatically the task.
  // A request can be manualy done to perform an FFT.
  if (app_settings_get_fft_period_hours() != 0){
    SCH_Modify_Task(task_perform_fft, SEC_TO_TICK(app_settings_get_fft_period_hours() * 3600), SEC_TO_TICK(app_settings_get_fft_period_hours() * 3600), false); // Reset the fft task.
  }
  app_hdw_wdt_kick();
}

static void task_download_data_ble(void)
{
  NRF_LOG_INFO("### DOWNLOAD DATA TASK ###");
  
  if (app_flash_enable()){
    uint32_t total_data_count = app_flash_get_data_session_count();
    uint8_t data_file_count = (uint8_t) (total_data_count / FILE_RECORD_COUNT) + 1;
  
    // If total data count is greater than 0, proceed to download.
    // If there's no data at all, there's no file too.
    if (total_data_count > 0){ 
      // For each file.
      for (uint8_t i = 0; i < data_file_count; ++i){
        uint32_t data_count = 0;

        // Open the data file and get the number of data inside the file.
        if (app_flash_download_data_file_start(i, &data_count)){
    
          uint8_t buffer[sizeof(struct app_packet_t)] = {0};
          for(uint32_t j = 0; j < data_count; ++j){
            // Downloading the packet at specific index.
            if (app_flash_download_data(j, buffer, sizeof(struct app_packet_t))){
              struct app_packet_t packet = {0};
              memcpy(&packet, buffer, sizeof(struct app_packet_t));
              NRF_LOG_INFO("Download %d: %d", j, packet.record_id);
              app_comm_send_packet(buffer, sizeof(struct app_packet_t));
            }
            else{
              // If there a problem downloading the data,
              // send a fail by BLE.
              NRF_LOG_ERROR("Unable to download data (index: %d)", j);
              app_comm_send_fail();
            }
            app_hdw_wdt_kick();
          }
          app_flash_download_data_file_stop();
        }
      }
    }
  }
  else{
    NRF_LOG_ERROR("Error initializing NOR flash.");
    app_comm_send_fail();
  }
  app_flash_disable();
  app_comm_send_response();
  app_hdw_gpio_low_power();
}

static void task_download_fft_ble(void)
{
  NRF_LOG_INFO("### DOWNLOAD FFT TASK ###");
  if (app_flash_enable()){
    uint32_t total_fft_count = app_flash_get_fft_session_count();
    uint8_t fft_file_count = (uint8_t) (total_fft_count / FFT_FILE_RECORD_COUNT) + 1;
  
    // If total data count is greater than 0, proceed to download.
    // If there's no fft at all, there's no file too.
    if (total_fft_count > 0){ 
      // For each fft file.
      for (uint8_t i = 0; i < fft_file_count; ++i){
        uint32_t fft_count = 0;
        // Open the data file and get the number of fft inside the file.
        if (app_flash_download_fft_file_start(i, &fft_count)){
          // For each fft.
          uint32_t fft_header_start = 0;
          for(uint32_t j = 0; j < fft_count; ++j){ 
            // Downloading the packet at specific index.
            // Getting fft header.
            struct app_fft_header_t header = {0};
            fft_header_start = (j * (FFT_SIZE + sizeof(struct app_fft_header_t)));
            if(app_flash_download_fft(fft_header_start, (uint8_t *)&header, sizeof(struct app_fft_header_t))){
              NRF_LOG_INFO("Download header %d: %d", j, header.fft_id);
              app_comm_send_packet((uint8_t *)&header, sizeof(struct app_fft_header_t));
            }
            else{
              // If there a problem downloading the data,
              // send a fail by BLE.
              NRF_LOG_ERROR("Unable to get FFT header (index: %d)", j);
              app_comm_send_fail();
            }
            
            // Getting FFT data.
            uint8_t buffer[BLE_FFT_PACKET_SIZE] = {0};
            uint32_t fft_data_start = 0;
            for(uint32_t k = 0; k < FFT_SIZE / BLE_FFT_PACKET_SIZE; ++k){
              fft_data_start = (j * (FFT_SIZE + sizeof(struct app_fft_header_t))) + sizeof(struct app_fft_header_t);
              if(app_flash_download_fft(fft_data_start + (k * BLE_FFT_PACKET_SIZE), buffer, sizeof(buffer))){
                NRF_LOG_INFO("Download fft data %d: %d", j, k);
                app_comm_send_packet(buffer, sizeof(buffer));
              }
              else{
                // If there a problem downloading the data,
                // send a fail by BLE.
                NRF_LOG_ERROR("Unable to get FFT data (index: %d)", k);
                app_comm_send_fail(); // Sending fail.
              }
            }
            app_hdw_wdt_kick();
          }
          app_flash_download_fft_file_stop();
        }
      }
    }
  }
  else{
    NRF_LOG_ERROR("Error initializing NOR flash.");
    app_comm_send_fail(); // Sending fail.
  }
  app_flash_disable();
  app_comm_send_response();
  app_hdw_gpio_low_power();
}

static void task_get_session_count(void)
{
  NRF_LOG_INFO("### GET SESSION COUNT TASK ###");
  uint16_t count = 0;
  if(app_flash_enable()){ // Enable NOR flash and littlefs.
    count = app_flash_get_data_session_count(); // Getting number of data sessions.
  }
  else{
    NRF_LOG_ERROR("Error initializing NOR flash.");
    app_comm_send_fail(); // Sending fail.
  }
  app_flash_disable(); // Disable NOR flash and littlefs.

  // Prepare data to send by BLE.
  uint8_t data[sizeof(count)] = {0};
  memcpy(data, &count, sizeof(count));

  // Sending result.
  app_comm_send_packet((uint8_t *)&count, sizeof(count)); 

  // Sending done.
  app_comm_send_response();
  
  // Kicking watchdog.
  app_hdw_wdt_kick();
}

static void task_get_fft_count(void)
{
  NRF_LOG_INFO("### GET FFT COUNT TASK ###");
  uint16_t count = 0;
  if(app_flash_enable()){ // Enable NOR flash and littlefs.
    count = app_flash_get_fft_session_count(); // Getting number of data sessions.
  }
  else{
    NRF_LOG_ERROR("Error initializing NOR flash.");
    app_comm_send_fail(); // Sending fail.
  }
  app_flash_disable(); // Disable NOR flash and littlefs.

  // Prepare data to send by BLE.
  uint8_t data[sizeof(count)] = {0};
  memcpy(data, &count, sizeof(count));

  // Sending result.
  app_comm_send_packet((uint8_t *)&count, sizeof(count)); 

  // Sending done.
  app_comm_send_response();
  
  // Kicking watchdog.
  app_hdw_wdt_kick();
}

static void task_erase_all(void)
{
  NRF_LOG_INFO("### ERASE ALL TASK ###");
  if(app_flash_enable()){ // Enable NOR flash and littlefs.
    app_flash_erase_all(); // Erasing all the NOR flash.
  }
  else{
    NRF_LOG_ERROR("Error initializing NOR flash.");
    app_comm_send_fail(); // Sending fail.
  }
  app_flash_disable(); // Disable NOR flash and littlefs.

  // Sending done.
  app_comm_send_response();

  // Kicking watchdog.
  app_hdw_wdt_kick();
}

static void task_erase_data(void)
{
  NRF_LOG_INFO("### ERASE DATA TASK ###");
  if(app_flash_enable()){ // Enable NOR flash and littlefs.
    app_flash_remove_data_sessions(); // Removing data files.
  }
  else{
    NRF_LOG_ERROR("Error initializing NOR flash.");
    app_comm_send_fail(); // Sending fail.
  }
  app_flash_disable(); // Disable NOR flash and littlefs.

  // Sending done.
  app_comm_send_response();

  // Kicking watchdog.
  app_hdw_wdt_kick();
}

static void task_erase_fft(void)
{
  NRF_LOG_INFO("### ERASE FFT TASK ###");
  if(app_flash_enable()){ // Enable NOR flash and littlefs.
    app_flash_remove_fft_sessions(); // Removing FFT files.
  }
  else{
    NRF_LOG_ERROR("Error initializing NOR flash.");
    app_comm_send_fail(); // Sending fail.
  }

  app_flash_disable(); // Disable NOR flash and littlefs.

  // Sending done.
  app_comm_send_response();

  // Kicking watchdog.
  app_hdw_wdt_kick();
}

static void task_flash_led(void)
{
  // Light the green LED for 25ms then close.
  nrf_gpio_pin_clear(GREEN_LED);
  app_hdw_set_green_led(true);
  nrf_delay_ms(25);
  app_hdw_set_green_led(false);
  app_hdw_wdt_kick();
}

static void task_advertise(void)
{
  if(!is_ble_user_connected() && !is_advertising)
  {
    advertising_start();
    is_advertising = true;
    SCH_Add_Task(task_stop_advertising, SEC_TO_TICK(0.5), 0, false);
  }
  app_hdw_wdt_kick();
}

static void task_stop_advertising(void)
{
  if(!is_ble_user_connected() && is_advertising)
  {
    advertising_stop();
    is_advertising = false;
  }
  app_hdw_wdt_kick();
}

static void task_power_off_device(void)
{
  NRF_LOG_INFO("### POWER OFF TASK ###");
  bool need_reset = false;
            
  // Disable soft device
  if (nrf_sdh_is_enabled()){
    APP_ERROR_CHECK(nrf_sdh_disable_request());
    need_reset = true;
  }
  
  // Write the device state in UICR.
  app_uicr_set(APP_NFC_WAKEUP_UICR_OFFSET, 0);
  
  NRF_LOG_INFO("The device will reset.");
  nrf_delay_ms(100);
  
  // Reset the device.
  if (need_reset){
    NVIC_SystemReset();
  }
}

static void task_data_request(void)
{
  NRF_LOG_INFO("### DATA REQUEST TASK ###");
  data_gathering(app_settings_get_record_duration_seconds()); 
  app_comm_send_response();
}

void setup_tasks(void)
{  
  // Background Tasks
  #if APP_BLE == 1
  SCH_Add_Task(task_advertise, SEC_TO_TICK(TASK_ADVERTISE_PERIOD), SEC_TO_TICK(TASK_ADVERTISE_PERIOD), false);
  #endif 

  // Green LED flashing periodically. Visual heart beat.
  SCH_Add_Task(task_flash_led, SEC_TO_TICK(TASK_LED_PERIOD), SEC_TO_TICK(TASK_LED_PERIOD), false);

  // Send heart beat through LoRaWAN
  SCH_Add_Task(task_send_heart_beat, 0, SEC_TO_TICK(app_settings_get_lora_heartbeat_period_minutes() * 60), false);

  // Get Data and Send it through LoRaWAN
  SCH_Add_Task(task_send_data, 0, SEC_TO_TICK(app_settings_get_record_period_minutes() * 60), false);

  // Getting and sending vibration data over LoRa
  if (TASK_VIBRATION_ANALYSIS_PERIOD != 0){
    SCH_Add_Task(task_vibration_analysis, SEC_TO_TICK(TASK_VIBRATION_ANALYSIS_PERIOD), SEC_TO_TICK(TASK_VIBRATION_ANALYSIS_PERIOD), false);
  }

  // Perform FFTs
  // If this config is set at 0, don't start automatically the task.
  // A request can be manualy done to perform an FFT.
  if (app_settings_get_fft_period_hours() != 0){
    SCH_Add_Task(task_perform_fft, SEC_TO_TICK(app_settings_get_fft_period_hours() * 3600), SEC_TO_TICK(app_settings_get_fft_period_hours() * 3600), false);
  }
}

void app_tasks_save_config(void)
{
  SCH_Add_Task(task_save_config, 0, 0, true);
}

void app_tasks_data_ble_download(void)
{
  SCH_Add_Task(task_download_data_ble, 0, 0, true);
}

void app_tasks_fft_ble_download(void)
{
  SCH_Add_Task(task_download_fft_ble, 0, 0, true);
}

void app_tasks_perform_fft(void)
{
  SCH_Add_Task(task_perform_fft, 0, 0, true);
}

void app_tasks_erase_data(void)
{
  SCH_Add_Task(task_erase_data, 0, 0, true);
}

void app_tasks_erase_fft(void)
{
  SCH_Add_Task(task_erase_fft, 0, 0, true);
}

void app_tasks_get_session_count(void)
{
  SCH_Add_Task(task_get_session_count, 0, 0, true);
}

void app_tasks_get_fft_count(void)
{
  SCH_Add_Task(task_get_fft_count, 0, 0, true);
}

void app_tasks_erase_all(void)
{
  SCH_Add_Task(task_erase_all, 0, 0, true);
}

void app_tasks_record_set_download_id(uint16_t session_id)
{
  session_to_download = session_id;
}

void app_tasks_power_off_device(void)
{
  SCH_Add_Task(task_power_off_device, 0, 0, true);
}

void app_tasks_request_data(void)
{
  SCH_Add_Task(task_data_request, 0, 0, true);
}
