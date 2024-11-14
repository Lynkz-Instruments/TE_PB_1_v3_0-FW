/**
 * @file app_uart.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "app_uart.h"

#include "nrf_serial.h"
#include "nrf_log.h"

#include "custom_board.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define APP_UART_VERBOSE 2

#define APP_UART_SERIAL_FIFO_TX_SIZE 512
#define APP_UART_SERIAL_FIFO_RX_SIZE 512
#define APP_UART_SERIAL_BUFF_TX_SIZE 1
#define APP_UART_SERIAL_BUFF_RX_SIZE 1

static void app_uart_sleep_handler(void);
static void app_uart_serial_event_handler(struct nrf_serial_s const *p_serial, nrf_serial_event_t event);

NRF_SERIAL_UART_DEF(app_uart_instance, 0);
NRF_SERIAL_QUEUES_DEF(app_uart_serial_queues, APP_UART_SERIAL_FIFO_TX_SIZE, APP_UART_SERIAL_FIFO_RX_SIZE);
NRF_SERIAL_BUFFERS_DEF(app_uart_serial_buffs, APP_UART_SERIAL_BUFF_TX_SIZE, APP_UART_SERIAL_BUFF_RX_SIZE);

NRF_SERIAL_CONFIG_DEF(app_uart_serial_config, NRF_SERIAL_MODE_IRQ, &app_uart_serial_queues, &app_uart_serial_buffs, app_uart_serial_event_handler, app_uart_sleep_handler);

// RX and tx pin are not reverted
NRF_SERIAL_DRV_UART_CONFIG_DEF(app_uart_config_lora, LORA_RX_PIN, LORA_TX_PIN, SERIAL_RTS_PIN, SERIAL_CTS_PIN, NRF_UART_HWFC_DISABLED , NRF_UART_PARITY_EXCLUDED, NRF_UART_BAUDRATE_115200, UART_DEFAULT_CONFIG_IRQ_PRIORITY);

volatile bool app_uart_rx_flag = false;
volatile bool app_uart_drv_error_flag = false;

static void app_uart_sleep_handler(void)
{
    __WFE();
    __SEV();
    __WFE();
}

void(*app_uart_rx_handler)(const char c) = NULL;

void app_uart_set_rx_callback(void(*rx_handler)(const char c))
{
  app_uart_rx_handler = rx_handler;
}

static void app_uart_serial_event_handler(struct nrf_serial_s const *p_serial, nrf_serial_event_t event) 
{
  switch(event)
  { 
    case NRF_SERIAL_EVENT_TX_DONE:
      #if APP_UART_VERBOSE >= 2
      // NRF_LOG_INFO("NRF_SERIAL_EVENT_TX_DONE");
      #endif
      break;
    case NRF_SERIAL_EVENT_DRV_ERR:
      #if APP_UART_VERBOSE >= 1
      NRF_LOG_ERROR("NRF_SERIAL_EVENT_DRV_ERR");
      #endif
      app_uart_drv_error_flag = true;
      break;

    case NRF_SERIAL_EVENT_FIFO_ERR:
      #if APP_UART_VERBOSE >= 1
      NRF_LOG_ERROR("NRF_SERIAL_EVENT_FIFO_ERR");
      #endif
      break;

    case NRF_SERIAL_EVENT_RX_DATA:
      app_uart_rx_flag = true;

      if( app_uart_rx_handler != NULL)
      {
        const char c = *(char*)p_serial->p_ctx->p_config->p_buffers->p_rxb;
        app_uart_rx_handler(c);
      }

      break;
  }
}

ret_code_t app_uart_init_lora(void)
{
  if (app_uart_instance.p_ctx->p_config) // Checks if module is initialized or not
  {
    const ret_code_t uninit_ret_code = nrf_serial_uninit(&app_uart_instance);
    if( uninit_ret_code != NRF_SUCCESS)
    {
      #if APP_UART_VERBOSE >= 1
      NRF_LOG_ERROR("Error With UART Uninit");
      #endif
      return uninit_ret_code;
    }
  }
 
  const ret_code_t init_ret_code = nrf_serial_init(&app_uart_instance, &app_uart_config_lora, &app_uart_serial_config);
  if( init_ret_code != NRF_SUCCESS)
  {
    #if APP_UART_VERBOSE >= 1
    NRF_LOG_ERROR("Error With UART Init");
    #endif
    return init_ret_code;
  }

  return NRF_SUCCESS;
}

ret_code_t app_uart_uninit(void)
{
  return nrf_serial_uninit(&app_uart_instance);
}

ret_code_t app_uart_flush(const uint32_t timeout_ms)
{
  return nrf_serial_flush(&app_uart_instance, timeout_ms);
}

ret_code_t app_uart_rx_drain(void)
{
  return nrf_serial_rx_drain(&app_uart_instance);
}

ret_code_t app_uart_write(void const * p_data, const uint32_t size, const uint32_t timeout_ms)
{
  return nrf_serial_write(&app_uart_instance, p_data, size, NULL, timeout_ms);
}

ret_code_t app_uart_read(void * p_data, uint32_t* size, const uint32_t timeout_ms)
{
  return nrf_serial_read(&app_uart_instance, p_data, *size, size, timeout_ms);
}

bool app_uart_get_and_clear_rx_flag(void)
{
  const bool flag = app_uart_rx_flag;
  app_uart_rx_flag = false;
  return flag;
}

bool app_uart_get_and_clear_drv_error_flag(void)
{
  const bool flag = app_uart_drv_error_flag;
  app_uart_drv_error_flag = false;
  return flag;
}

bool app_uart_get_drv_error_flag(void)
{
  return app_uart_drv_error_flag;
}

nrf_serial_t const * app_uart_get_instance(void)
{
  return &app_uart_instance;
}

nrf_serial_config_t const * app_uart_get_serial_config()
{
  return &app_uart_serial_config;
}

nrf_drv_uart_config_t const * app_uart_get_drv_config()
{
  return &app_uart_config_lora;
}
