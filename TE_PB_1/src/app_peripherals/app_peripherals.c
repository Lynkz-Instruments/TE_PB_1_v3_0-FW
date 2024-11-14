/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/

#include "app_peripherals.h"
#include "app_flash.h"
#include "app_settings.h"
#include "app_hardware.h"
#include "lynkz_utils.h"
#include "lynkz_crypto.h"
#include "app_uart_module.h"
#include "ST50H.h"

#define RECORD_FREQUENCY 10 // Hz

// Self test all peripherals.
bool app_peripherals_self_test(struct app_test_data_t * test_data)
{
  bool rslt = true;




  /******************************** Testing Antenna Device - START ****************************/
  // Try to Init Antenna Assembly
  if(app_antenna_init(0b11)){
    uint16_t temperature = 0;
    uint32_t frequency = 0;
    uint8_t error_mask = 0; // Currently unused with testing

    // Get temperature from antenna sensor
    app_antenna_get_temperature(&temperature);
    test_data->temp = (int16_t)temperature;

    // TODO Check if this is really the problem. Needed, otherwise the values are all 0.
    nrf_delay_ms(185);

    // Get resonant frequency values from both channels
    app_antenna_get_frequency(0, &frequency, &error_mask);
    test_data->channel_0 = frequency;

    app_antenna_get_frequency(1, &frequency, &error_mask);
    test_data->channel_1 = frequency;

    NRF_LOG_INFO("ANTENNA TEMPERATURE TESTING DATA: %d", test_data->temp);
    NRF_LOG_INFO("ANTENNA FREQUENCY CHAN. 0 TESTING DATA: %d", test_data->channel_0);
    NRF_LOG_INFO("ANTENNA FREQUENCY CHAN. 1 TESTING DATA: %d", test_data->channel_1);

    test_data->ldc1614_ok = true;
    test_data->tsys02d_ok = true;
    test_data->i2c_ok = true;
    NRF_LOG_INFO("Antenna Assembly Init Success......... 3/6 Passed!");
  }
  else{
    NRF_LOG_ERROR("Antenna Assembly Init Failed!");
    test_data->ldc1614_ok = false;
    test_data->tsys02d_ok = false;
    test_data->i2c_ok = false;
    rslt = false;
  }

  // Uninit Antenna Assembly
  app_antenna_uninit();
  /******************************** Testing Antenna Device - END *******************************/

  /************************************ Testing NFC pins - START **********************************/
  app_uicr_set_gpio_mode();
  NRF_LOG_INFO("NFC pin register %d", NRF_UICR->NFCPINS);
  
  nrf_gpio_cfg(NFC1_PIN, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg_input(NFC2_PIN, NRF_GPIO_PIN_NOPULL);

  nrf_gpio_pin_set(NFC1_PIN);

  nrf_delay_ms(10);

  if((bool)nrf_gpio_pin_read(NFC2_PIN) == true){
    NRF_LOG_INFO("NFC pins Success!....... 4/6 Test Passed");
    test_data->nfc_ok = true;
  }
  else{
    NRF_LOG_INFO("NFC pins not well soldered.");
    test_data->nfc_ok = false;
    rslt = false;
  }

  nrf_gpio_cfg(NFC1_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(NFC2_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);

  /************************************ Testing NFC pins - END ************************************/

  /************************************ Testing BLE - START ************************************/
  // Get BLE MAC
  #if APP_BLE == 1
  ble_gap_addr_t device_addr;
  ret_code_t     err_code;

  // Init SD and BLE Stack
  ble_init(nus_data_handler, (uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));

  // Change the BLE name for the proper one
  app_ble_init();
  advertising_start();
  app_task_set_advertising(true);

  err_code = sd_ble_gap_addr_get(&device_addr);
  APP_ERROR_CHECK(err_code);
  if(err_code == NRF_SUCCESS){
    // Copy ble address in struct
    memcpy(test_data->ble_device_addr, device_addr.addr, 6);
    NRF_LOG_INFO("BLE MAC Successfully read......... 5/6 Passed!");
    test_data->ble_ok = true;
  }
  else{
    NRF_LOG_INFO("Could not read BLE MAC..!");
    // Reset device address in struct
    memset(test_data->ble_device_addr, 0, 6);
    test_data->ble_ok = false;
    rslt = false;
  }
  #endif
  /************************************ Testing BLE - END *************************************/

  return rslt;
}

void app_peripherals_send_test_results(struct app_test_data_t * test_data)
{
  // Convert BLE MAC address to a string
  char ble_mac_s[13] = {0};
  bytesToHexString(test_data->ble_device_addr, 6, ble_mac_s, sizeof(ble_mac_s));
  
  // Convert BMI values to strings
  char accel_x_s[7] = {0};  // −32768 max value
  char accel_y_s[7] = {0};
  char accel_z_s[7] = {0};
  char gyro_x_s[7] = {0};
  char gyro_y_s[7] = {0};
  char gyro_z_s[7] = {0};
  itoa(test_data->accel_x, accel_x_s, 10);
  itoa(test_data->accel_y, accel_y_s, 10);
  itoa(test_data->accel_z, accel_z_s, 10);
  itoa(test_data->gyro_x, gyro_x_s, 10);
  itoa(test_data->gyro_y, gyro_y_s, 10);
  itoa(test_data->gyro_z, gyro_z_s, 10);

  // Convert Sensors values to strings
  char temp_s[9] = {0};           // 99999999
  char freq_channel_0_s[9] = {0}; // 99999999
  char freq_channel_1_s[9] = {0}; // 99999999
  itoa(test_data->temp, temp_s, 10);
  itoa(test_data->channel_0, freq_channel_0_s, 10);
  itoa(test_data->channel_1, freq_channel_1_s, 10);
  
  // LoRa
  char dev_addr[12] = {0};
  char apps_key[48] = {0};
  char nets_key[48] = {0};
  char app_eui[] = APP_LORA_APPEUI;
  char app_key[] = APP_LORA_APPKEY;
  generate_lora_keys(test_data->lora_deveui, dev_addr, apps_key, nets_key);
  remove_all_chars(dev_addr, ':');
  remove_all_chars(apps_key, ':');
  remove_all_chars(nets_key, ':');
  remove_all_chars(app_eui, ':');
  remove_all_chars(app_key, ':');

  // Log the test results
  NRF_LOG_INFO("Test results:");
  NRF_LOG_INFO("Accel_X: %s", accel_x_s);
  NRF_LOG_INFO("Accel_Y: %s", accel_y_s);
  NRF_LOG_INFO("Accel_Z: %s", accel_z_s);
  NRF_LOG_INFO("Gyro_X: %s", gyro_x_s);
  NRF_LOG_INFO("Gyro_Y: %s", gyro_y_s);
  NRF_LOG_INFO("Gyro_Z: %s", gyro_z_s);
  NRF_LOG_INFO("i2c_comm: %i", test_data->tsys02d_ok && test_data->ldc1614_ok);
  NRF_LOG_INFO("Temperature: %s", temp_s);
  NRF_LOG_INFO("Channel 0: %s", freq_channel_0_s);
  NRF_LOG_INFO("Channel 1: %s", freq_channel_1_s);
  NRF_LOG_INFO("Flash_Result: %i", test_data->flash_ok);
  NRF_LOG_INFO("NFC Result: %i", test_data->nfc_ok);
  NRF_LOG_INFO("Serial_Comm: %i", test_data->uart_ok);
  NRF_LOG_INFO("DevEUI: %s", test_data->lora_deveui);
  NRF_LOG_INFO("AppKey: %s", app_key);
  NRF_LOG_INFO("DevAddr: %s", dev_addr);
  NRF_LOG_INFO("AppSession Key: %s", apps_key);
  NRF_LOG_INFO("NetSession Key: %s", nets_key);
  NRF_LOG_INFO("BLE_MAC: %s", ble_mac_s);
  NRF_LOG_INFO("FW_Version: %s", FW_VERSION);

  // Send the struct in text over UART (JSON format)
  char tosend[512] = {0}; // 512 characters should be enough for the entire struct
  // Add everything by strcating it
  // IMU_Data block
  strcat(tosend, "^{\n\"IMU_Data\":{\n");
  strcat(tosend, "\"Accel_X\":");
  strcat(tosend, accel_x_s);
  strcat(tosend, ",\n\"Accel_Y\":");
  strcat(tosend, accel_y_s);
  strcat(tosend, ",\n\"Accel_Z\":");
  strcat(tosend, accel_z_s);
  strcat(tosend, ",\n\"Gyro_X\":");
  strcat(tosend, gyro_x_s);     
  strcat(tosend, ",\n\"Gyro_Y\":");
  strcat(tosend, gyro_y_s);
  strcat(tosend, ",\n\"Gyro_Z\":");
  strcat(tosend, gyro_z_s);
  strcat(tosend, "\n},\n");
  // Sensor_Data block
  strcat(tosend, "\"Sensor_Data\":{\n");
  strcat(tosend, "\"temp\":");
  strcat(tosend, temp_s);
  strcat(tosend, ",\n\"channel_0\":");
  strcat(tosend, freq_channel_0_s);
  strcat(tosend, ",\n\"channel_1\":");
  strcat(tosend, freq_channel_1_s);
  strcat(tosend, "\n},\n");
  // Flash_Result block
  strcat(tosend, "\"Flash_Result\":");
  strcat(tosend, test_data->flash_ok ? "\"PASS\",\n" : "\"FAIL\",\n");
  // NFC block
  strcat(tosend, "\"NFC_pins\":");
  strcat(tosend, test_data->nfc_ok ? "\"PASS\",\n" : "\"FAIL\",\n");
  // LoRa/UART block
  strcat(tosend, "\"LoRa_Data\":{\n");
  strcat(tosend, "\"DevEUI\":\"");
  strcat(tosend, test_data->lora_deveui);
  strcat(tosend, "\",\n\"AppEUI\":\"");
  strcat(tosend, app_eui);
  strcat(tosend, "\",\n\"AppKey\":\"");
  strcat(tosend, app_key);
  strcat(tosend, "\",\n\"DevAddr\":\"");
  strcat(tosend, dev_addr);
  strcat(tosend, "\",\n\"AppSKey\":\"");
  strcat(tosend, apps_key);
  strcat(tosend, "\",\n\"NetSKey\":\"");
  strcat(tosend, nets_key);
  strcat(tosend, "\"\n},\n");
  // BLE Mac
  strcat(tosend, "\"BLE_MAC\":\"");
  strcat(tosend, ble_mac_s);
  strcat(tosend, "\",\n");
  // FW Version
  strcat(tosend, "\"FW_Version\":\"");
  strcat(tosend, FW_VERSION);
  strcat(tosend, "\"\n}\r");
  
  // Send the JSON to the test board
  app_uart_module_init_test_board();
  nrf_delay_ms(100);
  app_uart_module_write((uint8_t *)tosend, strlen(tosend), NULL);
  nrf_delay_ms(100);
  app_uart_module_uninit(); 
}

void app_peripherals_get_data(struct app_packet_t * data, uint16_t record_id, uint16_t record_time_sec)
{
  // IMPORTANT: Flash must be enable before using this function.

  if (data != NULL){
    // Get bitmask of enabled channels, and activate only the needed ones
    uint8_t bitmask = app_settings_get_ch_enabled_bitmask();

    struct app_packet_t sens_data = {0};
    sens_data.record_id = record_id;

    //uint32_t wait = (uint32_t)(1.0f / (float)RECORD_FREQUENCY) * 1000.0f;
    uint32_t wait = 1000; // ms
    uint32_t count = (uint32_t)(record_time_sec / (wait / 1000.0f));
    
    uint64_t temp_total = 0;
    uint16_t temperature = 0;

    uint64_t freq_0_total = 0; 
    uint64_t freq_1_total = 0; 
    uint8_t freq_0_error_mask = 0;
    uint8_t freq_1_error_mask = 0;

    int64_t accel_mod_total = 0; 
    int64_t gyro_mod_total = 0;
     
    // Let the LDC1614 Settle
    nrf_delay_ms(200);

    NRF_LOG_INFO("RECORDING STARTED...");
    for(uint32_t i=0; i < count; i++){
        
      NRF_LOG_INFO("Record %d", i);

      sens_data.err_chan_0 = freq_0_error_mask;
      sens_data.err_chan_1 = freq_1_error_mask;
      

      // Save this packet in flash
      if (!app_flash_record_data_packet((uint8_t *)&sens_data, sizeof(sens_data))){
        NRF_LOG_ERROR("Error writing data packet to file.");
      }

      app_hdw_wdt_kick();

      // Wait Sampling Rate Delay
      nrf_delay_ms(wait - 100); // TODO: I will probably change this to be more precise with a timer.
    }
    NRF_LOG_INFO("RECORDING DONE!");

    // Calculate average values and store them in struct
    data->record_id = record_id;
    data->temp = (uint16_t)((float)temp_total/(float)count);
    data->freq_chan_0 = (uint32_t)((float)freq_0_total/(float)count);
    data->freq_chan_1 = (uint32_t)((float)freq_1_total/(float)count);
    data->accel_mod = (uint16_t)((float)accel_mod_total/(float)count);
    data->gyro_mod = (uint16_t)((float)gyro_mod_total/(float)count);
    data->err_chan_0 = sens_data.err_chan_0;
    data->err_chan_1 = sens_data.err_chan_1;
  
    NRF_LOG_INFO("Average Temperature (raw): %d",(uint16_t) data->temp);
    NRF_LOG_INFO("Average Freq. Coil Sensor (raw): %d", data->freq_chan_0);
    NRF_LOG_INFO("Average Freq. Capacitive Sensor (raw): %d", data->freq_chan_1);
    NRF_LOG_INFO("Average Accel. Mod. (0.1mg): %d", data->accel_mod);
    NRF_LOG_INFO("Average Gyro. Mod. (deg/s): %d", data->gyro_mod);
    NRF_LOG_INFO("Channel 0 Error bitmask: %02x", freq_0_error_mask);
    NRF_LOG_INFO("Channel 1 Error bitmask: %02x", freq_1_error_mask);


  }
}

void app_peripherals_system_off(void)
{
  // Power all peripherals down
  app_hdw_pwr_flash_bmi(false);
  app_hdw_pwr_lora(false);
  app_hdw_pwr_antenna(false);
}
