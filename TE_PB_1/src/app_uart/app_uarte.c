/**
 * @file app_uarte.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "app_uarte.h"

//#include "nrf_serial.h"
#include "nrf_log.h"
#include "nrf_queue.h"

#include "custom_board.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define APP_UARTE_VERBOSE 1

uint8_t * buffer;

static void app_uart_sleep_handler(void);
static void app_uarte_event_handler(void * context, nrf_libuarte_async_evt_t * p_evt);

// NRF_LIBUARTE_DRV_DEFINE(app_uarte_drv, 0, 4);
NRF_LIBUARTE_ASYNC_DEFINE(app_uarte_drv, 0, 3, NRF_LIBUARTE_PERIPHERAL_NOT_USED, 4, 255, 3);

volatile bool app_uarte_rx_flag = false;

void(*app_uarte_rx_handler)(const char c) = NULL;

void app_uarte_set_rx_callback(void(*rx_handler)(const char c))
{
  app_uarte_rx_handler = rx_handler;
}


void app_uarte_set_buffer(uint8_t * buffer)
{
  
}

static void app_uarte_event_handler(void * context, nrf_libuarte_async_evt_t * p_evt) 
{
    switch(p_evt->type)
    {
      case NRF_LIBUARTE_ASYNC_EVT_TX_DONE:
        #if APP_UARTE_VERBOSE >= 1
        NRF_LOG_INFO("NRF_LIBUARTE_ASYNC_EVT_TX_DONE");
        #endif
        break;
      
      case NRF_LIBUARTE_ASYNC_EVT_ERROR:
        #if APP_UARTE_VERBOSE >= 1
        NRF_LOG_ERROR("NRF_LIBUARTE_ASYNC_EVT_ERROR");
        #endif
        break;

      case NRF_LIBUARTE_ASYNC_EVT_OVERRUN_ERROR:
        #if APP_UARTE_VERBOSE >= 1
        NRF_LOG_ERROR("NRF_LIBUARTE_ASYNC_EVT_OVERRUN_ERROR");
        #endif
        break;

      case NRF_LIBUARTE_ASYNC_EVT_RX_DATA:
        #if APP_UARTE_VERBOSE >= 1
        NRF_LOG_INFO("NRF_LIBUARTE_ASYNC_EVT_RX_DATA");
        #endif
        app_uarte_rx_flag = true;
        if( app_uarte_rx_handler != NULL)
        {
          const char c = p_evt->data.rxtx.p_data[0];
          NRF_LOG_HEXDUMP_INFO(p_evt->data.rxtx.p_data, p_evt->data.rxtx.length);
          app_uarte_rx_handler(c);
          nrf_libuarte_async_rx_free(&app_uarte_drv, p_evt->data.rxtx.p_data, p_evt->data.rxtx.length);
        }
        break;
    }
}

ret_code_t app_uarte_init_lora(void)
{
  nrf_libuarte_async_config_t libuarte_async_config = {
          .tx_pin       = LORA_TX_PIN,
          .rx_pin       = LORA_RX_PIN,
          .cts_pin      = SERIAL_CTS_PIN,
          .rts_pin      = SERIAL_RTS_PIN,
          .baudrate     = NRF_UARTE_BAUDRATE_115200,
          .parity       = NRF_UARTE_PARITY_EXCLUDED,
          .hwfc         = NRF_UARTE_HWFC_DISABLED,
          .timeout_us   = 100,
          .int_prio     = APP_IRQ_PRIORITY_LOW_MID,
  };

  const ret_code_t init_ret_code = nrf_libuarte_async_init(&app_uarte_drv, &libuarte_async_config, &app_uarte_event_handler, (void *)&app_uarte_drv);
  if( init_ret_code != NRF_SUCCESS)
  {
    #if APP_UARTE_VERBOSE >= 1
    NRF_LOG_ERROR("Error With UART Init");
    #endif
    return init_ret_code;
  }

  return NRF_SUCCESS;
}

void app_uarte_uninit(void)
{
  nrf_libuarte_async_uninit(&app_uarte_drv);
}


nrf_libuarte_async_t const * app_uarte_get_instance(void)
{
  return &app_uarte_drv;
}
