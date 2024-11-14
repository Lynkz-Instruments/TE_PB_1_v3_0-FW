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

  app_hdw_wdt_kick();
  app_hdw_gpio_low_power(); // Sleep
}

static void task_perform_fft(void)
{
}

static void task_vibration_analysis(void)
{
}

static void data_gathering(uint16_t record_duration)
{
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
  
  NRF_LOG_INFO("The device will reset.");
  nrf_delay_ms(100);

  if (need_reset){ // When changing config with BLE. Reset.
    NVIC_SystemReset();
  }
  
  SCH_Modify_Task(task_send_heart_beat, 0, SEC_TO_TICK(app_settings_get_lora_heartbeat_period_minutes() * 60), true); // Reset the heartbeat task.
  SCH_Modify_Task(task_send_data, SEC_TO_TICK(app_settings_get_record_period_minutes() * 60), SEC_TO_TICK(app_settings_get_record_period_minutes() * 60), false); // Reset the data record task.

  app_hdw_wdt_kick();
}

static void task_download_data_ble(void)
{
}

static void task_download_fft_ble(void)
{
}

static void task_get_session_count(void)
{
}

static void task_get_fft_count(void)
{
}

static void task_erase_all(void)
{
}

static void task_erase_data(void)
{
}

static void task_erase_fft(void)
{
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
