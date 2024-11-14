/**
 * @file app_lora.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "app_lora.h"

#include <stdarg.h>
#include "sdk_errors.h"
#include "nrf_serial.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "app_hardware.h"
#include "app_uart_module.h"
#include "app_saadc.h"
#include "custom_board.h"
#include "lynkz_utils.h"
#include "app_communication.h"
#include "lynkz_crypto.h"
#include "app_settings.h"
#include "lora_at_master.h"
#include "nrf5_utils.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define APP_LORA_VERBOSE 1

#define APP_LORA_SUPERCAP_TIMEOUT_MS (3000)
static const float APP_LORA_SUPERCAP_RECOV_V = 28;

static bool lora_joined = false;
static volatile bool LoraAnswerPending = false;

// Byte0: pkt_type (a,b,c,...) Byte1: Axis (0x7b for 10-axis data) Byte2-3: Chunk No. Bytes4-123: 10-axis data for 6 samples
uint8_t lora_pkt_dr2[125] = {0};
static char responseBuffer[512] = {0};

// PRIVATE FUNCTIONS PROTOTYPES //

static ret_code_t Send(uint8_t * payload, uint8_t payload_size, bool confirmed, uint8_t port, bool check_downlink);

void app_lora_rx_uart_handler(const char c) 
{
  if (c == LORA_AT_RESP_END && !LoraAnswerPending){ // Detect first 0x0A char
    LoraAnswerPending = true;
    return;
  }

  if (c == LORA_AT_RESP_END && LoraAnswerPending){ // Detect second 0x0A char
    LoraAnswerPending = false;
    lora_at_set_lora_new_answer(); // Set flag to break waiting loop lora_at_master driver.
  }
}

ret_code_t app_lora_init(bool uartInit)
{
  if (uartInit) {
    app_lora_init_uart();
  }

  lora_at_answer rslt = OK;
  char dev_addr[12] = {0};
  char apps_key[48] = {0};
  char nets_key[48] = {0};
  char deveui[24] = {0};
  
  nrf_gpio_cfg(LORA_RST_PIN, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_H0H1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_pin_clear(LORA_RST_PIN);
  nrf_delay_ms(300);
  nrf_gpio_pin_set(LORA_RST_PIN);

  // Let ST50H boot message happen before initializing serial
  nrf_delay_ms(2000);
  
  // LoRa default setup.
  // Generate LoRa keys from deveui
  lora_at_get_deui(deveui);
  generate_lora_keys(deveui, dev_addr, apps_key, nets_key);
  remove_all_chars(dev_addr, ':');
  remove_all_chars(apps_key, ':');
  remove_all_chars(nets_key, ':');
  
  // Set keys
  lora_at_set_appskey(apps_key);
  lora_at_set_nwkskey(nets_key);
  lora_at_set_daddr(dev_addr);

  // Set keys in settings
  app_settings_set_lora_keys(deveui, dev_addr, apps_key, nets_key);
  
  // Join the LNS.
  rslt = lora_at_join(ABP);
  if (rslt != JOINED){
    NRF_LOG_ERROR("LoRa status unjoined.");
    return NRF_ERROR_INVALID_STATE;
  }

  // Set default used channel.
  lora_at_disable_channels(8, 63);

  // Init process done.
  lora_joined = true;

  return NRF_SUCCESS;
}

ret_code_t app_lora_wakeup(void)
{
  app_lora_init_uart();
  return NRF_SUCCESS;
}

ret_code_t app_lora_sleep(void)
{
  return app_lora_uninit_uart();
}

ret_code_t app_lora_getdeveui(char * deveui)
{
  lora_at_answer rslt = OK;
  rslt = lora_at_get_deui(deveui);
  if (rslt != OK){
    return NRF_ERROR_INVALID_STATE;
  }

  if (strlen(deveui) == 0){
    return NRF_ERROR_INVALID_STATE;
  }

  remove_all_chars(deveui, ':');

  return NRF_SUCCESS;
}

void app_lora_send_start(void)
{
  uint8_t resetreason = (uint8_t)NRF5_UTILS_GetResetReasons();
  uint8_t powerOnPkt[1] = {resetreason};  // Send reset reason when restarting
  Send(powerOnPkt, sizeof(powerOnPkt), false, 60, true);

  return;
}

void app_lora_send_version(void)
{
  char firm_ver[] = FW_VERSION;
  get_versions(firm_ver, &lora_pkt_dr2[0], &lora_pkt_dr2[1], &lora_pkt_dr2[2]);
  Send(lora_pkt_dr2, 3, false, 76, true);
}

void app_lora_send_heartbeat(uint8_t * data, uint8_t len, bool check_downlink)
{
  memcpy(&lora_pkt_dr2[0], data, len);
  Send(lora_pkt_dr2, len, false, 62, check_downlink);
}

void app_lora_send_vibration_data_pkt(uint16_t period, uint8_t * data, uint8_t len, bool check_downlink)
{
  memcpy(&lora_pkt_dr2[0], (uint8_t *)&period, sizeof(period));
  memcpy(&lora_pkt_dr2[2], data, len);
  Send(lora_pkt_dr2, sizeof(period) + len, false, 64, check_downlink);
}

void app_lora_send_data_pkt(uint8_t * data, uint8_t len, bool check_downlink)
{
  memcpy(&lora_pkt_dr2[0], data, len);
  Send(lora_pkt_dr2, len, false, 61, check_downlink);
}

void app_lora_send_fft_pkt(uint16_t fft_id, uint8_t chunk_id, int16_t gain, uint8_t freq, uint8_t * data, uint8_t len, bool check_downlink)
{
  memcpy(&lora_pkt_dr2[0], &fft_id, sizeof(uint16_t));
  lora_pkt_dr2[2] = chunk_id;
  memcpy(&lora_pkt_dr2[3], &gain, sizeof(int16_t));
  lora_pkt_dr2[5] = freq;
  memcpy(&lora_pkt_dr2[6], data, len);

  Send(lora_pkt_dr2, sizeof(fft_id) + sizeof(chunk_id) + sizeof(gain) + sizeof(freq) + len, false, 67, check_downlink);
}

static void app_lora_print(const char * msg, ...)
{
  char txt[512];
  va_list ap;
  va_start(ap, msg);
  va_end(ap);
  vsprintf (txt,msg, ap);

  uint16_t txt_len = strlen(txt);
  NRF_LOG_INFO("%s", txt)
}

ret_code_t app_lora_init_uart(void)
{
  const ret_code_t uart_init_error = app_uart_module_init_lora();
  if(uart_init_error != NRF_SUCCESS){
    #if APP_LORA_VERBOSE >= 1
    NRF_LOG_ERROR("Error with app_uart_init_lora, %d", uart_init_error);
    #endif

    return uart_init_error;
  }

  // Set the serial interface.
  struct lora_at_serial_interface interface = {
    .init_func = app_uart_module_init_lora,
    .uninit_func = app_uart_module_uninit,
    .read_func = app_uart_module_read,
    .write_func = app_uart_module_write,
    .flush_func = app_uart_module_flush,
    .debug_print_func = app_lora_print,
    .delay_ms_func = nrf_delay_ms,
    .delay_us_func = nrfx_coredep_delay_us
  };
  lora_at_set_serial_interface(&interface);

  // Set the uart rx callback.
  app_uart_module_set_rx_callback(app_lora_rx_uart_handler);

  return NRF_SUCCESS;
}

ret_code_t app_lora_uninit_uart(void)
{
  // Clear the rx callback function.
  app_uart_module_set_rx_callback(NULL);

  const ret_code_t uart_uninit_error = app_uart_module_uninit();
  if(uart_uninit_error != NRF_SUCCESS){
    #if APP_LORA_VERBOSE >= 1
    NRF_LOG_ERROR("Error with app_uart_uninit, %d", uart_uninit_error);
    #endif

    return uart_uninit_error;
  }

  app_hdw_disconnect_lora_uart();
  return NRF_SUCCESS;
}

bool app_lora_joined(void)
{
  return lora_joined;
}

// PRIVATE FUNCTIONS //

static ret_code_t Send(uint8_t * payload, uint8_t payload_size, bool confirmed, uint8_t port, bool check_downlink)
{
  char message[LORA_AT_CMD_SIZE_DR2] = {0};
  lora_at_downlink_t downlink;
  lora_at_answer rslt = OK;
  
  // Convert bytes to hex string.
  bytesToHexString(payload, payload_size, message, 2 * payload_size);

  // Sending the uplink.
  rslt = lora_at_send_uplink(port, confirmed, confirmed ? 3 : 1, message, &downlink);
  if (rslt == ERROR){
    NRF_LOG_ERROR("Error sending with LoRa");
  } else if (rslt == UNJOINED) {
    NRF_LOG_ERROR("Unjoined, reinitialize LoRa radio");
    app_lora_init(false);
    // Only reattempt once, to not loop
    rslt = lora_at_send_uplink(port, confirmed, confirmed ? 3 : 1, message, &downlink);
  }
  
  // Debug LED signal when downlink received.
  if (rslt == DOWNLINK_RX || rslt == SEND_CONFIRMED){
    app_hdw_set_blue_led(true);
    nrf_delay_ms(25);
    app_hdw_set_blue_led(false);
    nrf_delay_ms(25);
    app_hdw_set_blue_led(true);
    nrf_delay_ms(25);
    app_hdw_set_blue_led(false);
  }

  // Check and process downlink.
  if (check_downlink && downlink.size != -1 && rslt == DOWNLINK_RX){
    char resp[256];
    strcpy(resp, downlink.payload);
    app_comm_lora_process((uint8_t *)resp, downlink.size * 2);
  }

  // Wait for supercap to recharge
  nrf_saadc_value_t battery = 0;
  float battery_v = 0.0f;
  uint32_t scapCounter = 0;

  app_saadc_init();
  app_saadc_get_channel(3, &battery);
  battery_v = ((battery * 0.6f) / 4096.0f) * 6.0f;
  while (battery_v * 10 < APP_LORA_SUPERCAP_RECOV_V) {
    app_saadc_get_channel(3, &battery);
    battery_v = ((battery * 0.6f) / 4096.0f) * 6.0f;
    nrf_delay_ms(10);
    scapCounter += 10;
    if (scapCounter > APP_LORA_SUPERCAP_TIMEOUT_MS) {
      break;
    }
  }
  app_saadc_uninit();

  return NRF_SUCCESS;
}
