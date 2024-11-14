/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/

#include "scheduler.h"
#include "ble_nrf.h"

#include "app_hardware.h"
#include "app_ble.h"
#include "app_peripherals.h"
#include "app_settings.h"
#include "app_tasks.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define MAIN_VERBOSE 2

// 0 -> No
// 1 -> Yes
#define FLASH_RESET 0

/**@brief Function for application main entry.
 */

int main(void)
{
  // TODO: Remove before deployment
  //app_uicr_set(UICR_DEVICE_STATE_ID, 1);
  //app_uicr_set(UICR_PANEL_NO_LSB_ID, (uint32_t) 0x00000002);
  //app_uicr_set(UICR_PANEL_NO_MSB_ID, (uint32_t) 0x00000000);
  //app_uicr_set(UICR_PCBA_NO_ID, (uint32_t) 0x00000000);
  //app_uicr_set(UICR_HWVER_MIN_ID, (uint32_t) 0x00000002);
  //app_uicr_set(UICR_HWVER_MAJ_ID, (uint32_t) 0x00000001);
  //app_uicr_set(UICR_BATCHNO_LSB_0_ID, (uint32_t) 0x00000020);
  //app_uicr_set(UICR_BATCHNO_LSB_1_ID, (uint32_t) 0x00000012);
  //app_uicr_set(UICR_BATCHNO_MSB_2_ID, (uint32_t) 0x00000022);
  //app_uicr_set(UICR_BATCHNO_MSB_3_ID, (uint32_t) 0x00000020);

  //app_uicr_set(APP_NFC_WAKEUP_UICR_OFFSET, (uint32_t) 0x00000001 );

  // Hardware init
  app_hdw_init();

  // Self test if device is on test board.
  if (app_hdw_is_on_test_board()){
    nrf_delay_ms(3000);

    struct app_test_data_t test_data = {0};
    app_peripherals_self_test(&test_data);
    
    while(1){
      app_hdw_set_leds(false, false, false);
      nrf_delay_ms(2000);
      app_hdw_set_leds(true, false, false);
      nrf_delay_ms(25);
      app_hdw_wdt_kick();
    }
  }
  app_hdw_disconnect_test_board_detect();

  // Getting config from NOR flash.
  #if FLASH_RESET > 0

  #endif

  #if MAIN_VERBOSE >= 2
  // Showing configuration for debugging purpose.
  app_settings_show_config();
  #endif

  #if APP_BLE == 1
  // Init SD and BLE Stack
  ble_init(nus_data_handler, (uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));

  // Change the BLE name for the proper one
  app_ble_init();
  advertising_start();
  app_task_set_advertising(true);
  #endif

  // Start application execution. 
  NRF_LOG_INFO("INIT DONE: SMARTLINER APP STARTED!");
  


  if (!is_ble_user_connected()){
    advertising_stop();
    app_task_set_advertising(false);
  }

  // Setup Tasks based on memory config
  setup_tasks();

  while(true)
  {
    SCH_Dispatch_Tasks();
    app_hdw_wdt_kick();

    /* Clear exceptions and PendingIRQ from the FPU unit */
    __set_FPSCR(__get_FPSCR()  & ~(0x0000009F));      
    (void) __get_FPSCR();
    NVIC_ClearPendingIRQ(FPU_IRQn);

    /* Call SoftDevice Wait For event */
    sd_app_evt_wait();
  }
}
