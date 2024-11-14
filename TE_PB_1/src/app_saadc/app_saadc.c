/**
 * @file app_saadc.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief
 * @version 1.0
 * @date 2023-06-05
 *
 * @copyright Copyright (c) Lynkz Instruments Inc. Amos 2022
 *
 */

#include "app_saadc.h"
#include "nrf_drv_saadc.h"

static void event_handler(nrfx_saadc_evt_t const *p_event) {
  /*
  ret_code_t err_code;
  switch (p_event->type) {
    case NRFX_SAADC_EVT_DONE:
    NRF_LOG_INFO("DONE. Sample[0] = %i", p_event->data.done.p_buffer[0]);
    // Add code here to process the input. If the processing is time consuming 
    // execution should be deferred to the main context
    break;
    case NRFX_SAADC_EVT_BUF_REQ:
    // Set up the next available buffer
    err_code = nrfx_saadc_buffer_set(&samples[next_free_buf_index()][0], SAADC_BUF_SIZE);
    APP_ERROR_CHECK(err_code);
    break;
   }
   */
}

ret_code_t app_saadc_init(void)
{
  ret_code_t err_code;
  nrfx_saadc_config_t saadc_config = {.resolution = NRF_SAADC_RESOLUTION_12BIT,
                                      .oversample = NRF_SAADC_OVERSAMPLE_DISABLED,
                                      .interrupt_priority = 6,
                                      .low_power_mode = true};
  // Initialize the SAADC
  err_code = nrfx_saadc_init(&saadc_config, &event_handler);
  APP_ERROR_CHECK(err_code);

  // Create configurations for the channels
  nrf_saadc_channel_config_t battery_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
  
  // Initialize each channel
  err_code = nrfx_saadc_channel_init(0, &battery_config);
  APP_ERROR_CHECK(err_code);
  return err_code;
}

void app_saadc_get_channel(uint8_t channel, nrf_saadc_value_t * value)
{
  nrfx_saadc_sample_convert(channel, value);
}

void app_saadc_uninit(void)
{
  nrfx_err_t err_code;
  err_code = nrfx_saadc_channel_uninit(0);
  APP_ERROR_CHECK(err_code);
  nrfx_saadc_uninit();
}