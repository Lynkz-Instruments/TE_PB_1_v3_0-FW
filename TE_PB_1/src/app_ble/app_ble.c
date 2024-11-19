/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/

#include "app_ble.h"
#include "stdarg.h"
#include "app_tasks.h"
#include "scheduler.h"
#include "app_hardware.h"
#include "app_peripherals.h"
#include "app_communication.h"
#include <stdio.h>

#define MAX_ADVERTISING_NAME_LENGTH 26

bool ble_nus_comm_ok = false;

void nus_data_handler(ble_nus_evt_t * p_evt)
{
  if(p_evt->type == BLE_NUS_EVT_RX_DATA){
    uint16_t data_len = p_evt->params.rx_data.length;
    uint8_t const* data_ptr = p_evt->params.rx_data.p_data;

    app_comm_process(data_ptr, data_len);

  }
  else if(p_evt->type == BLE_NUS_EVT_COMM_STARTED){
    ble_nus_comm_ok = true;
  }
  else if(p_evt->type == BLE_NUS_EVT_COMM_STOPPED){
    ble_nus_comm_ok = false;
  }
}

void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
  uint32_t err_code;

  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      NRF_LOG_INFO("Connected");
      ble_user_connected = true;
      m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      err_code = nrf_ble_qwr_conn_handle_assign(p_m_qwr, m_conn_handle);
      APP_ERROR_CHECK(err_code);
      break;

    case BLE_GAP_EVT_DISCONNECTED:
      NRF_LOG_INFO("Disconnected");
      ble_user_connected = false;
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
      advertising_stop();
      app_task_set_advertising(false);
      break;

    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
    {
      NRF_LOG_DEBUG("PHY update request.");
      ble_gap_phys_t const phys =
      {
          .rx_phys = BLE_GAP_PHY_AUTO,
          .tx_phys = BLE_GAP_PHY_AUTO,
      };
      err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
      APP_ERROR_CHECK(err_code);
    } break;

    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
      // Pairing not supported
      err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
      APP_ERROR_CHECK(err_code);
      break;

    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
      // No system attributes have been stored.
      err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
      APP_ERROR_CHECK(err_code);
      break;

    case BLE_GATTC_EVT_TIMEOUT:
      // Disconnect on GATT Client timeout event.
      err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                       BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
      APP_ERROR_CHECK(err_code);
      break;

    case BLE_GATTS_EVT_TIMEOUT:
      // Disconnect on GATT Server timeout event.
      err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                       BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
      APP_ERROR_CHECK(err_code);
      break;

    default:
      // No implementation needed.
      break;
  }
}

void app_ble_init(void)
{
  char ble_name[MAX_ADVERTISING_NAME_LENGTH] = {0};
  uint8_t ble_name_len = 0;
  char ptr_no[5];
  uint16_t panel_no = app_uicr_get(UICR_PANEL_NO_LSB_ID) + (app_uicr_get(UICR_PANEL_NO_MSB_ID) << 8);
  uint8_t batch_no_data[4] = {app_uicr_get(UICR_BATCHNO_MSB_3_ID), app_uicr_get(UICR_BATCHNO_MSB_2_ID), app_uicr_get(UICR_BATCHNO_LSB_1_ID), app_uicr_get(UICR_BATCHNO_LSB_0_ID)};
  char batch_no_str[9]; // ddccbbaa
  bytesToHexString(batch_no_data, sizeof(batch_no_data), batch_no_str, sizeof(batch_no_str));

  strcat(ble_name, "TE_PB");
  strcat(ble_name, batch_no_str);
  strcat(ble_name, "_");
  sprintf(ptr_no, "%03u", panel_no);
  strcat(ble_name, ptr_no);
  strcat(ble_name, "-");
  sprintf(ptr_no, "%u", (uint8_t)app_uicr_get(UICR_PCBA_NO_ID));
  strcat(ble_name, ptr_no);

  if (strlen(ble_name) > MAX_ADVERTISING_NAME_LENGTH)
  {
    ble_set_name((uint8_t *)ble_name, MAX_ADVERTISING_NAME_LENGTH);
  }
  else
  {
    ble_set_name((uint8_t *)ble_name, strlen(ble_name));
  }
  // Need to be called again after changin BLE name
  advertising_init();

  // Set TX power to maximum
  //sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, BLE_GAP_TX_POWER_ROLE_ADV, RADIO_TXPOWER_TXPOWER_Pos4dBm);
}
