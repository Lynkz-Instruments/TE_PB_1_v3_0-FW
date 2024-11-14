/**
 * @file app_i2c.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief
 * @version 1.0
 * @date 2023-01-12
 *
 * @copyright Copyright (c) Lynkz Instruments Inc. Amos 2022
 *
 */

#ifndef APP_I2C_H
#define APP_I2C_H

#include <stdbool.h>
#include <stdint.h>

typedef enum app_i2c_xfer_result_t {
    APP_I2C_XFER_RESULT_SUCCESS = 0,
    APP_I2C_XFER_RESULT_PARAM_ERROR,
    APP_I2C_XFER_RESULT_TIMEOUT,
    APP_I2C_XFER_RESULT_ERROR,
    APP_I2C_XFER_RESULT_MAX
} app_i2c_xfer_result_t;

/**
 * @brief Initialize the i2c.
 * 
 * @return true -> Succes
 * @return false -> Error
 */
bool app_i2c_init(void);

/**
 * @brief Uninitialize the i2c
 */
void app_i2c_uninit(void);

/**
 * @brief Send value over i2c
 */
app_i2c_xfer_result_t app_i2c_tx(uint8_t address, const uint8_t * data_p, uint8_t len);

/**
 * @brief Get value over i2c
 */
app_i2c_xfer_result_t app_i2c_rx(uint8_t address, uint8_t * data_p, uint8_t len);

#endif // APP_I2C_H
