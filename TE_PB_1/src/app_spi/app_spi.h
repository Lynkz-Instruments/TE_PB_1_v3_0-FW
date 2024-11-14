/**
 * @file app_spi.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief
 * @version 1.0
 * @date 2023-01-11
 *
 * @copyright Copyright (c) Lynkz Instruments Inc. Amos 2022
 *
 */

#ifndef APP_SPI_H
#define APP_SPI_H

// This module require the app_hardware and the custom board
#include "nrf_drv_spi.h"

// SPI instance
extern const nrf_drv_spi_t app_spi_instance;

/**
 * @brief Initialize the SPI.
 * 
 * @return NRF_SUCCES -> Succes
 * @return NRF_ERRORx -> Error
 */
ret_code_t app_spi_init(void);

/**
 * @brief Uninitialize the SPI
 */
void app_spi_uninit(void);

#endif // APP_SPI_H