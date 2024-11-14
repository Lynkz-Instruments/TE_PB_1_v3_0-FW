/**
 * @file app_i2c.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief
 * @version 1.0
 * @date 2023-01-12
 *
 * @copyright Copyright (c) Lynkz Instruments Inc. Amos 2022
 *
 */

#include "app_i2c.h"

#include "nrfx_twi.h"

#include "sdk_errors.h"

#include "custom_board.h"
#include "app_hardware.h"

// DEFINES AND CONST //

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define APP_I2C_VERBOSE 1
#define APP_I2C_TIMEOUT_US (1000) // In us
const nrfx_twi_t app_i2c_instance = NRFX_TWI_INSTANCE(0);

// GLOBAL VARIABLES //
static bool initialization_done = false;
static bool i2c_xfer_done = false;  // True when xfer is done
static bool i2c_xfer_nack = false;  // True if transfer NACK

// PRIVATE PROTOTYPES //
void i2c_event_hdlr(const nrfx_twi_evt_t *p_event, void *p_context);

// PUBLIC FUNCTIONS //
bool app_i2c_init(void)
{
  if(initialization_done){ return true; }
  ret_code_t err_code;

  app_hdw_pwr_antenna(true);

  // Prepare configuration.
  const nrfx_twi_config_t twi_config = {
    .scl = I2CM0_SCL_PIN,
    .sda = I2CM0_SDA_PIN,
    .frequency = NRFX_TWI_DEFAULT_CONFIG_FREQUENCY,
    .interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY,
    .hold_bus_uninit = NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT,
  };

  // Init i2c interface.
  const ret_code_t init_error = nrfx_twi_init(&app_i2c_instance, &twi_config, &i2c_event_hdlr, NULL);
  if( init_error != NRF_SUCCESS){ return false; }

  // Enable i2c interface.
  nrfx_twi_enable(&app_i2c_instance);

  initialization_done = true;
  return true;
}

void app_i2c_uninit(void)
{
  if(!initialization_done){ return; }
  
  // Disable i2c interface.
  nrfx_twi_disable(&app_i2c_instance);

  // Uninit i2c interface.
  nrfx_twi_uninit(&app_i2c_instance);

  // app_hdw_disconnect_i2c();
  app_hdw_pwr_antenna(false);

  initialization_done = false;
}

app_i2c_xfer_result_t app_i2c_tx(uint8_t address, const uint8_t * data_p, uint8_t len) {
  if (NULL == data_p) {
    return APP_I2C_XFER_RESULT_PARAM_ERROR;
  }
  i2c_xfer_done = false;
  i2c_xfer_nack = false;
  nrfx_twi_xfer_desc_t xfer_desc = NRFX_TWI_XFER_DESC_TX(address, (uint8_t*)data_p, len);
  const ret_code_t err_code = nrfx_twi_xfer(&app_i2c_instance, &xfer_desc, NRFX_TWI_XFER_TX);

  if (err_code == NRF_ERROR_INTERNAL || err_code == NRF_ERROR_DRV_TWI_ERR_OVERRUN ||
        err_code == NRF_ERROR_DRV_TWI_ERR_ANACK || err_code == NRF_ERROR_DRV_TWI_ERR_DNACK){
    return APP_I2C_XFER_RESULT_ERROR;
  }
  uint16_t time = 0;
  while(1) {
    if (time > APP_I2C_TIMEOUT_US) {
      return APP_I2C_XFER_RESULT_TIMEOUT;
    }
    if (i2c_xfer_done) {
      i2c_xfer_done = false;
      break;
    }
    time++;
    nrf_delay_us(1);
  }
  if (i2c_xfer_nack) {
    i2c_xfer_nack = false;
    return APP_I2C_XFER_RESULT_ERROR;
  }

  // NRF_LOG_DEBUG("Waited for: %d us", time);
  // NRF_LOG_DEBUG("TX Transfer done successfully!");
  return APP_I2C_XFER_RESULT_SUCCESS;
}

app_i2c_xfer_result_t app_i2c_rx(uint8_t address, uint8_t * data_p, uint8_t len) {
  if (NULL == data_p) {
    return APP_I2C_XFER_RESULT_PARAM_ERROR;
  }
  i2c_xfer_done = false;
  i2c_xfer_nack = false;
  nrfx_twi_xfer_desc_t xfer_desc = NRFX_TWI_XFER_DESC_RX(address, data_p, len);
  const ret_code_t err_code = nrfx_twi_xfer(&app_i2c_instance, &xfer_desc, NRFX_TWI_XFER_RX);
  if (err_code == NRF_ERROR_INTERNAL || err_code == NRF_ERROR_DRV_TWI_ERR_OVERRUN ||
        err_code == NRF_ERROR_DRV_TWI_ERR_ANACK || err_code == NRF_ERROR_DRV_TWI_ERR_DNACK){
    return APP_I2C_XFER_RESULT_ERROR;
  }

  uint16_t time = 0;
  while(1) {
    if (time > APP_I2C_TIMEOUT_US) {
      return APP_I2C_XFER_RESULT_TIMEOUT;
    }
    if (i2c_xfer_done) {
      i2c_xfer_done = false;
      break;
    }
    time++;
    nrf_delay_us(1);
  }
  if (i2c_xfer_nack) {
    i2c_xfer_nack = false;
    return APP_I2C_XFER_RESULT_ERROR;
  }

  // NRF_LOG_DEBUG("Waited for: %d us", time);
  // NRF_LOG_DEBUG("RX Transfer done successfully!");
  return APP_I2C_XFER_RESULT_SUCCESS;
}

// PRIVATE FUNCTIONS //
void i2c_event_hdlr(const nrfx_twi_evt_t *p_event, void *p_context) {
  if (NULL == p_event) {
    return;
  }

  switch (p_event->type) {
    case NRFX_TWI_EVT_DONE:
      #if APP_I2C_VERBOSE >= 2
      NRF_LOG_DEBUG("NRFX_TWI_EVT_DONE");
      #endif
      break;
    case NRFX_TWI_EVT_ADDRESS_NACK:
      #if APP_I2C_VERBOSE >= 1
      NRF_LOG_ERROR("NRFX_TWI_EVT_ADDRESS_NACK");
      #endif
      i2c_xfer_nack = true;
      break;
    case NRFX_TWI_EVT_DATA_NACK:
      #if APP_I2C_VERBOSE >= 2
      NRF_LOG_ERROR("NRFX_TWI_EVT_DATA_NACK");
      #endif
      i2c_xfer_nack = true;
      break;
    case NRFX_TWI_EVT_OVERRUN:
      #if APP_I2C_VERBOSE >= 2
      NRF_LOG_DEBUG("NRFX_TWI_EVT_OVERRUN");
      #endif
      break;
  }

  #if APP_I2C_VERBOSE >= 2
  NRF_LOG_DEBUG("Transfer type: %d", p_event->xfer_desc.type);
  #endif

  i2c_xfer_done = true;

  return;
}
