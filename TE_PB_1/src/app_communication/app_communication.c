/**
 * @file app_communication.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "app_communication.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "ble_nrf.h"
#include "app_settings.h"
#include "app_ble.h"
#include "app_flash.h"
#include "app.h"
#include "lynkz_utils.h"
#include "nrf5_utils.h"
#include "app_tasks.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define APP_COMMUNICATION_VERBOSE 2

#define NUS_COMMAND_MAX_COUNT_BYTES 20 
#define MAX_LORA_DOWNLINK_CHAR    NUS_COMMAND_MAX_COUNT_BYTES * 2

#define SET_CONFIG_COMMMAND       0xA2
#define DOWNLOAD_DATA_COMMAND     0xA3
#define DOWNLOAD_FFT_COMMAND      0xAD
#define GET_SESSION_COUNT_COMMAND 0xA4
#define GET_FFT_COUNT_COMMAND     0xAE
#define GET_DEVICE_INFO_COMMAND   0xA5
#define ERASE_MEM_COMMMAND        0xA6
#define GET_CONFIG_COMMMAND       0xA7
#define ERASE_DATA_COMMAND        0xA8
#define ERASE_FFT_COMMAND         0xAC
#define POWER_OFF_COMMAND         0x73
#define GET_LORA_KEYS             0xAB
#define REQUEST_DATA_COMMAND      0xAF
#define PERFORM_FFT_COMMAND       0xA9
#define RESTART_COMMAND           0x72

#define OK_RESPONSE               0xE0
#define DONE_RESPONSE             0xE1
#define FAILED_RESPONSE           0xE2
#define EMPTY_RESPONSE            0xE3

// Declarations
static void app_comm_set_config(uint8_t * command);
static void app_comm_get_config(void);
static void app_comm_download_data(void);
static void app_comm_download_fft(void);
static void app_comm_get_session_count(void);
static void app_comm_get_fft_count(void);
static void app_comm_get_device_info(void);
static void app_comm_power_off_device(void);
static void app_comm_erase_all(void);
static void app_comm_erase_data(void);
static void app_comm_erase_fft(void);
static void app_comm_request_data(void);
static void app_comm_remblai_get_lora_keys(void);
static void app_comm_perform_fft(void);
static void app_comm_restart_device(void);

void app_comm_lora_process(uint8_t * data, uint8_t len)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Downlink received.");
  #endif

  uint16_t size = len / 2;
  uint8_t bytearray[MAX_LORA_DOWNLINK_CHAR/2];
  // Verify number of characters
  if (len % 2 != 0 || len > MAX_LORA_DOWNLINK_CHAR){
    #if APP_COMMUNICATION_VERBOSE >= 1
    NRF_LOG_ERROR("Error in the downlink received.");
    #endif
    return;
  }
  // Convert characters to data
  for(int i = 0; i < size; i++){
    sscanf(data + 2 * i, "%02X", &bytearray[i]);
  }
  app_comm_process(bytearray, size);
}

void app_comm_process(uint8_t const* data, uint16_t len)
{
  if(data != NULL){
    #if APP_COMMUNICATION_VERBOSE >= 2
    NRF_LOG_INFO("Received data:");
    NRF_LOG_HEXDUMP_INFO(data, len);
    #endif
    
    if (len > NUS_COMMAND_MAX_COUNT_BYTES){
      #if APP_COMMUNICATION_VERBOSE >= 1
      NRF_LOG_ERROR("Invalid command.");
      #endif
      return;
    }
    char command[NUS_COMMAND_MAX_COUNT_BYTES] = {0};
    memcpy(command, data, len);
    
    switch(command[0]){
      case SET_CONFIG_COMMMAND:
        app_comm_set_config((uint8_t *)command);
        break;
      case DOWNLOAD_DATA_COMMAND:
        app_comm_download_data();
        break;
      case DOWNLOAD_FFT_COMMAND:
        app_comm_download_fft();
        break;
      case GET_SESSION_COUNT_COMMAND:
        app_comm_get_session_count();
        break;
      case GET_FFT_COUNT_COMMAND:
        app_comm_get_fft_count();
        break;
      case GET_DEVICE_INFO_COMMAND:
        app_comm_get_device_info();
        break;
      case ERASE_MEM_COMMMAND:
        app_comm_erase_all();
        break;
      case GET_CONFIG_COMMMAND:
        app_comm_get_config();
        break;
      case POWER_OFF_COMMAND:
        app_comm_power_off_device();
        break;
      case REQUEST_DATA_COMMAND:
        app_comm_request_data();
        break;
      case GET_LORA_KEYS:
        app_comm_remblai_get_lora_keys();
        break;
      case ERASE_DATA_COMMAND:
        app_comm_erase_data();
        break;
      case ERASE_FFT_COMMAND:
        app_comm_erase_fft();
        break;
      case PERFORM_FFT_COMMAND:
        app_comm_perform_fft();
        break;
      case RESTART_COMMAND:
        app_comm_restart_device();
        break;
      default:
        #if APP_COMMUNICATION_VERBOSE >= 1
        NRF_LOG_ERROR("Invalid command.");
        app_comm_send_fail();
        #endif
        break;
    }
  }
}

static void app_comm_set_config(uint8_t * command)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Setting configuration.");
  #endif
  
  struct app_config_t cfg;
  memcpy(&cfg, &command[2], sizeof(struct app_config_t));
  app_settings_set_configuration((uint8_t *)&cfg);
  app_tasks_save_config();
}

static void app_comm_download_data(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Downloading data.");
  #endif

  app_comm_send_ack();
}

static void app_comm_download_fft(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Downloading fft.");
  #endif

  app_comm_send_ack();
}

static void app_comm_get_session_count(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Getting session count.");
  #endif
  
  app_comm_send_ack();
}

static void app_comm_get_fft_count(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Getting fft count.");
  #endif
  
  app_comm_send_ack();
}

static void app_comm_get_device_info(void)
{
  char firm_ver[] = FW_VERSION;
  struct app_version_packet_t data_ver = {0};
  uint8_t buffer[12] = {0};

  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Getting device informations.");
  #endif

  app_comm_send_ack();
  
  // Device number
  buffer[0] = (uint8_t) app_uicr_get(UICR_PANEL_NO_LSB_ID);
  buffer[1] = (uint8_t) app_uicr_get(UICR_PANEL_NO_MSB_ID);
  buffer[2] = (uint8_t) app_uicr_get(UICR_PCBA_NO_ID);
  
  // Firmware version 
  get_versions(firm_ver, &data_ver.major, &data_ver.minor, &data_ver.patch);
  buffer[3] = data_ver.patch;
  buffer[4] = data_ver.minor;
  buffer[5] = data_ver.major;
  
  // Hardware version
  buffer[6] = (uint8_t) app_uicr_get(UICR_HWVER_MIN_ID);
  buffer[7] = (uint8_t) app_uicr_get(UICR_HWVER_MAJ_ID);

  // Batch number
  buffer[8] = (uint8_t) app_uicr_get(UICR_BATCHNO_LSB_0_ID);
  buffer[9] = (uint8_t) app_uicr_get(UICR_BATCHNO_LSB_1_ID);
  buffer[10] = (uint8_t) app_uicr_get(UICR_BATCHNO_MSB_2_ID);
  buffer[11] = (uint8_t) app_uicr_get(UICR_BATCHNO_MSB_3_ID);

  app_comm_send_packet((uint8_t *)&buffer, sizeof(buffer));
  app_comm_send_response();
}

static void app_comm_erase_all(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Erase memory.");
  #endif

  app_comm_send_ack();
}

static void app_comm_erase_data(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Erase data.");
  #endif

  app_comm_send_ack();
}

static void app_comm_erase_fft(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Erase fft.");
  #endif

  app_comm_send_ack();
}

static void app_comm_get_config(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Getting config.");
  #endif

  app_comm_send_ack();

  struct app_config_t dev_cfg = {0};
  app_settings_get_configuration((uint8_t *)&dev_cfg);
  app_comm_send_packet((uint8_t *)&dev_cfg, sizeof(struct app_config_t));
  app_comm_send_response();
}

static void app_comm_power_off_device(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Power OFF device.");
  #endif

  app_comm_send_ack();

  app_tasks_power_off_device();
}

static void app_comm_request_data(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Request data.");
  #endif

  app_comm_send_ack();
  app_tasks_request_data();
}

static void app_comm_remblai_get_lora_keys(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Get LoRa keys.");
  #endif

  char deveui[17] = {0};
  char dev_addr[9] = {0};
  char apps_key[33] = {0};
  char nets_key[33] = {0};

  app_comm_send_ack();
  app_settings_get_lora_keys(deveui, dev_addr, apps_key, nets_key);

  app_comm_send_packet((uint8_t *)deveui, sizeof(deveui));
  app_comm_send_packet((uint8_t *)dev_addr, sizeof(dev_addr));
  app_comm_send_packet((uint8_t *)apps_key, sizeof(apps_key));
  app_comm_send_packet((uint8_t *)nets_key, sizeof(nets_key));

  app_comm_send_response();
}

static void app_comm_perform_fft(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Perform FFT.");
  #endif

  app_comm_send_ack();
}

static void app_comm_restart_device(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Restart the device.");
  #endif

  NVIC_SystemReset();
}

void app_comm_send_packet(uint8_t * data, uint8_t len)
{
  if (ble_nus_comm_ok && is_ble_user_connected()){
    while(send_nus(data, len) != NRF_SUCCESS){/*Waiting for a success.*/}
  }
}

void app_comm_send_ack(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Send acknowledge.");
  #endif
  uint8_t data = OK_RESPONSE;
  app_comm_send_packet(&data, sizeof(uint8_t));
}

void app_comm_send_response(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Send response.");
  #endif
  uint8_t data = DONE_RESPONSE;
  app_comm_send_packet(&data, sizeof(uint8_t));
}

void app_comm_send_fail(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Send fail response.");
  #endif
  uint8_t data = FAILED_RESPONSE;
  app_comm_send_packet(&data, sizeof(uint8_t));
}

void app_comm_send_empty(void)
{
  #if APP_COMMUNICATION_VERBOSE >= 2
  NRF_LOG_INFO("Send empty response.");
  #endif
  uint8_t data = EMPTY_RESPONSE;
  app_comm_send_packet(&data, sizeof(uint8_t));
}