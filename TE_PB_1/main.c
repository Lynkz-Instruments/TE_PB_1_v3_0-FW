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
#include "app_uart_module.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define MAIN_VERBOSE 2

// 0 -> No
// 1 -> Yes
#define FLASH_RESET 0

/**@brief Function for application main entry.
 */
#include "nrf_uarte.h"
#include "nrf_ppi.h"
#include "nrf_drv_ppi.h"
#include "app_error.h"

#define UART_RX_PIN  8  // Pin de réception
#define UART_TX_PIN  6  // Pin de transmission

static uint8_t rx_buffer[1];  // Buffer pour un caractère reçu
static uint8_t tx_buffer[1];  // Buffer pour un caractère à envoyer


#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_ppi.h"
#include "app_error.h"

#define UART_RX_PIN  8  // Pin de réception UART
#define UART_TX_PIN  6  // Pin de transmission UART

void ppi_uart_init(void)
{
    ret_code_t err_code;

    // 1. Initialiser GPIOTE
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    // 2. Configurer GPIOTE pour la broche RX (événement sur changement d'état)
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false); // Sensibilité au changement d'état
    in_config.pull = NRF_GPIO_PIN_NOPULL;

    err_code = nrf_drv_gpiote_in_init(UART_RX_PIN_NUMBER, &in_config, NULL);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(UART_RX_PIN_NUMBER, true); // Activer les événements sur RX

    // 3. Configurer GPIOTE pour la broche TX (tâche de sortie)
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(true);
    err_code = nrf_drv_gpiote_out_init(UART_TX_PIN_NUMBER, &out_config);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_out_task_enable(UART_TX_PIN_NUMBER); // Activer les tâches sur TX

    // 4. Initialiser PPI
    nrf_ppi_channel_t ppi_channel;
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    // 5. Assigner PPI : relier l'événement RX à la tâche TX
    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(ppi_channel,
                                          nrf_drv_gpiote_in_event_addr_get(UART_RX_PIN_NUMBER),
                                          nrf_drv_gpiote_out_task_addr_get(UART_TX_PIN_NUMBER));
    APP_ERROR_CHECK(err_code);

    // 6. Activer le canal PPI
    err_code = nrf_drv_ppi_channel_enable(ppi_channel);
    APP_ERROR_CHECK(err_code);
}


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

  ppi_uart_init();

  // Start application execution. 
  NRF_LOG_INFO("INIT DONE: SMARTLINER APP STARTED!");
  


  if (!is_ble_user_connected()){
    advertising_stop();
    app_task_set_advertising(false);
  }

  // Setup Tasks based on memory config
  setup_tasks();

  //uart_configure_pins(UART_RX_PIN_NUMBER, UART_TX_PIN_NUMBER);

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
