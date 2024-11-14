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

