/**
 * @file app_uart_module.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "app_uart_module.h"

#include "app_uart.h"
#include "app_error.h"
#include "nrf_uart.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "nrfx_uarte.h"

#include "custom_board.h"
#include "app.h"

#define UART_RX_BUF_SIZE 512  // UART RX buffer size.

static nrfx_uarte_t uarte = NRFX_UARTE_INSTANCE(0); // Instance unique UARTE
static uint8_t rx_buffer[UART_RX_BUF_SIZE];
static uint8_t tx_buffer[UART_RX_BUF_SIZE];
static uint32_t rx_counter = 0;
static bool tx_ready = false;

void uart_event_handler_1(nrfx_uarte_event_t const *p_event, void *p_context);
void uart_event_handler_2(nrfx_uarte_event_t const *p_event, void *p_context);

void(*app_uart_module_rx_handler)(const char c) = NULL;

void app_uart_module_set_rx_callback(void(*rx_handler)(const char c))
{
  app_uart_module_rx_handler = rx_handler;
}

void app_uart_module_event_handler(app_uart_evt_t * p_event)
{
  char ch;
  switch(p_event->evt_type){
    case APP_UART_COMMUNICATION_ERROR:
      break;
    case APP_UART_FIFO_ERROR:
      APP_ERROR_HANDLER(p_event->data.error_code);
      break;
    case APP_UART_DATA:
      if(app_uart_get((uint8_t *)&ch) == NRF_SUCCESS){
        if(app_uart_module_rx_handler){
          // Send the character to the rx handler.
          app_uart_module_rx_handler(ch);
          // Add to the buffer too.
          rx_buffer[rx_counter] = (uint8_t)ch;
          rx_counter++;
        }
      }
      break;
    case APP_UART_DATA_READY:
      break;
    case APP_UART_TX_EMPTY:
      tx_ready = true;
      break;
  }
}

ret_code_t app_uart_module_write(const uint8_t * p_data, const uint32_t size, uint32_t timeout_ms)
{
  ret_code_t err_code;
  for(int i = 0; i < size; ++i){
    err_code = app_uart_put(p_data[i]);
    APP_ERROR_CHECK(err_code);
    while(!tx_ready){};
    tx_ready = false;
  }
  return NRF_SUCCESS;
}

ret_code_t app_uart_module_read(uint8_t * p_data, uint32_t size, uint32_t timeout_ms)
{
  memcpy(p_data, rx_buffer, rx_counter);
  return NRF_SUCCESS;
}

ret_code_t app_uart_module_flush(uint32_t timeout_ms)
{
  memset(rx_buffer, 0, UART_RX_BUF_SIZE);
  rx_counter = 0;
  return NRF_SUCCESS;
}

ret_code_t app_uart_module_uninit(void)
{
  return (ret_code_t)app_uart_close();
}

// Fonction pour configurer les pins de l'UART
void uart_configure_pins(uint32_t rx_pin, uint32_t tx_pin) {
    //nrfx_uarte_uninit(&uarte); // Désinitialisation
    nrfx_uarte_config_t config = NRFX_UARTE_DEFAULT_CONFIG;
    config.pselrxd = rx_pin;
    config.pseltxd = tx_pin;
    config.hwfc = NRF_UARTE_HWFC_DISABLED; // Pas de contrôle de flux
    config.baudrate = 9600;
    APP_ERROR_CHECK(nrfx_uarte_init(&uarte, &config, NULL));
}

// Fonction pour lire les données et transférer
void uart_transfer(uint32_t src_rx_pin, uint32_t dst_tx_pin) {
    uart_configure_pins(src_rx_pin, dst_tx_pin); // Configurer pour la source/destination
    nrfx_uarte_rx(&uarte, rx_buffer, 1);        // Lire un octet (bloquant dans cet exemple)
    nrfx_uarte_tx(&uarte, rx_buffer, 1);        // Envoyer cet octet
}

void uart_send_byte(char byte[]) {
    // Envoi d'un byte
    APP_ERROR_CHECK(nrfx_uarte_tx(&uarte, &byte, 16));
}