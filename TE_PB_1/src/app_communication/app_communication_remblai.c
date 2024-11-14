/**
 * @file app_communication_remblai.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-03-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "app_communication_remblai.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "ble_nrf.h"
#include "app_settings.h"
#include "app_ble.h"
#include "app_flash.h"
#include "app_tasks_remblai.h"
#include "app.h"
#include "lynkz_utils.h"
#include "nrf5_utils.h"
#include "app_settings_remblai.h"
#include "app_communication.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define APP_COMMUNICATION_REMBLAI_VERBOSE 2

#define SET_CONFIG_COMMMAND       0xA2
#define DOWNLOAD_SESSION_COMMAND  0xA3
#define GET_SESSION_COUNT_COMMAND 0xA4
#define GET_DEVICE_INFO_COMMAND   0xA5
#define ERASE_MEM_COMMMAND        0xA6
#define GET_CONFIG_COMMMAND       0xA7
#define RECORD_COMMMAND           0xA8
#define ACTIVATE_COMMAND          0xA9
#define LORA_DOWNLOAD_COMMAND     0xAA

#define OK_RESPONSE               0xE0
#define DONE_RESPONSE             0xE1
#define FAILED_RESPONSE           0xE2
#define EMPTY_RESPONSE            0xE3

// Declarations
static void app_comm_remblai_set_config(uint8_t * command);
static void app_comm_remblai_get_config(void);
static void app_comm_remblai_download_session(uint8_t * command);
static void app_comm_remblai_lora_download_session(uint8_t * command);
static void app_comm_remblai_get_session_count(void);
static void app_comm_remblai_get_device_info(void);
static void app_comm_remblai_record(void);
static void app_comm_remblai_activate_deactivate_device(uint8_t * command);
static void app_comm_remblai_erase_all(void);

void app_comm_remblai_process(uint8_t const* data, uint16_t len)
{
  if(data != NULL){
    #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
    NRF_LOG_INFO("Received data:");
    NRF_LOG_HEXDUMP_INFO(data, len);
    #endif
    
    if (len > NUS_COMMAND_MAX_COUNT_BYTES){
      #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 1
      NRF_LOG_ERROR("Invalid command.");
      #endif
      return;
    }
    char command[NUS_COMMAND_MAX_COUNT_BYTES] = {0};
    memcpy(command, data, len);
    
    switch(command[0]){
      case SET_CONFIG_COMMMAND:
        app_comm_remblai_set_config(command);
        break;
      case DOWNLOAD_SESSION_COMMAND:
        app_comm_remblai_download_session(command);
        break;
      case LORA_DOWNLOAD_COMMAND:
        app_comm_remblai_lora_download_session(command);
        break;
      case GET_SESSION_COUNT_COMMAND:
        app_comm_remblai_get_session_count();
        break;
      case GET_DEVICE_INFO_COMMAND:
        app_comm_remblai_get_device_info();
        break;
      case ERASE_MEM_COMMMAND:
        app_comm_remblai_erase_all();
        break;
      case GET_CONFIG_COMMMAND:
        app_comm_remblai_get_config();
        break;
      case RECORD_COMMMAND:
        app_comm_remblai_record();
        break;
      case ACTIVATE_COMMAND:
        app_comm_remblai_activate_deactivate_device(command);
        break;
      default:
        #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 1
        NRF_LOG_ERROR("Invalid command.");
        #endif
        break;
    }
  }
}

static void app_comm_remblai_set_config(uint8_t * command)
{
  #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
  NRF_LOG_INFO("Setting configuration.");
  #endif
  
  struct app_config_remblai_t cfg;
  memcpy(&cfg, &command[2], sizeof(struct app_config_remblai_t));
  app_settings_set_configuration((uint8_t *)&cfg);
  app_tasks_remblai_save_config();
}

static void app_comm_remblai_download_session(uint8_t * command)
{
  #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
  NRF_LOG_INFO("Downloading a session.");
  #endif

  app_comm_send_ack();

  uint16_t session_id;
  memcpy(&session_id, &command[1], sizeof(uint16_t));
  app_tasks_remblai_record_set_download_id(session_id);
  app_tasks_remblai_record_download();
}

static void app_comm_remblai_lora_download_session(uint8_t * command)
{
  #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
  NRF_LOG_INFO("Downloading a session using LoRa.");
  #endif

  app_comm_send_ack();

  uint16_t session_id;
  memcpy(&session_id, &command[1], sizeof(uint16_t)); 
  app_tasks_remblai_record_set_download_id(session_id);
  app_tasks_remblai_lora_download();
}

static void app_comm_remblai_get_session_count(void)
{
  #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
  NRF_LOG_INFO("Getting session count.");
  #endif
  
  app_comm_send_ack();

  app_tasks_remblai_record_get_session_count();
}

static void app_comm_remblai_get_device_info(void)
{
  char firm_ver[] = FW_VERSION;
  struct app_version_packet_t data_ver = {0};
  uint8_t buffer[12] = {0};

  #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
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

static void app_comm_remblai_erase_all(void)
{
  #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
  NRF_LOG_INFO("Erase memory.");
  #endif

  app_comm_send_ack();

  app_tasks_remblai_erase_all();
}

static void app_comm_remblai_get_config(void)
{
  #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
  NRF_LOG_INFO("Getting config.");
  #endif

  app_comm_send_ack();

  struct app_config_remblai_t dev_cfg = {0};
  get_remblai_configuration((uint8_t *)&dev_cfg);
  app_comm_send_packet((uint8_t *)&dev_cfg, sizeof(struct app_config_remblai_t));
  app_comm_send_response();
}

static void app_comm_remblai_record(void)
{
  #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
  NRF_LOG_INFO("Start record.");
  #endif

  app_comm_send_ack();

  app_tasks_remblai_set_comm_request(true);
  app_tasks_remblai_record_start();
}

static void app_comm_remblai_activate_deactivate_device(uint8_t * command)
{
  #if APP_COMMUNICATION_REMBLAI_VERBOSE >= 2
  NRF_LOG_INFO("Activate/Deactivate.");
  #endif

  app_comm_send_ack();

  if (command[1] == 1){
    app_tasks_remblai_activate();
  }
  else if (command[1] == 0){
    app_tasks_remblai_deactivate();
  }
}
