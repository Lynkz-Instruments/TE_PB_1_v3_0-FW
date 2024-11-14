/**
 * @file app_spi.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief
 * @version 1.0
 * @date 2023-01-11
 *
 * @copyright Copyright (c) Lynkz Instruments Inc. Amos 2022
 *
 */

#include "app_spi.h"

#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "stdint.h"
#include "sdk_errors.h"

#include "custom_board.h"
#include "app_hardware.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define APP_SPI_VERBOSE 1

const nrf_drv_spi_t app_spi_instance = NRF_DRV_SPI_INSTANCE(2);
static bool initialization_done = false;

ret_code_t app_spi_init(void)
{
  if(initialization_done){ return NRF_SUCCESS; }
  
  // Power ON devices
  app_hdw_pwr_flash_bmi(true);

  nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;

  spi_config.ss_pin = NRF_DRV_SPI_PIN_NOT_USED;

  // Flash and IMU SPI Chip Select
  nrf_gpio_cfg_output(SPIM1_CSB_FLASH_PIN);
  nrf_gpio_cfg_output(SPIM1_CSB_IMU_PIN);
  nrf_gpio_pin_set(SPIM1_CSB_FLASH_PIN);
  nrf_gpio_pin_set(SPIM1_CSB_IMU_PIN);

  spi_config.miso_pin = SPIM1_MISO_PIN;
  spi_config.mosi_pin = SPIM1_MOSI_PIN;
  spi_config.sck_pin  = SPIM1_SCK_PIN;

  spi_config.frequency = NRF_DRV_SPI_FREQ_4M;
  spi_config.mode = NRF_DRV_SPI_MODE_3;

  const ret_code_t init_error = nrf_drv_spi_init(&app_spi_instance, &spi_config, NULL, NULL);
  if( init_error != NRF_SUCCESS){ return init_error; }

  initialization_done = true;

  return NRF_SUCCESS;
}

void app_spi_uninit(void)
{
  if(!initialization_done){ return; }

  nrf_drv_spi_uninit(&app_spi_instance);
  initialization_done = false;

  app_hdw_disconnect_spi();
  app_hdw_pwr_flash_bmi(false);
}
