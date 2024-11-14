/**
 * @file app_saadc.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief
 * @version 1.0
 * @date 2023-06-05
 *
 * @copyright Copyright (c) Lynkz Instruments Inc. Amos 2022
 *
 */

#ifndef APP_SAADC_H
#define APP_SAADC_H

#include <stdint.h>
#include <stdbool.h>
#include "sdk_errors.h"

#include "nrfx_saadc.h"

ret_code_t app_saadc_init(void);
void app_saadc_uninit(void);
void app_saadc_get_channel(uint8_t channel, nrf_saadc_value_t * value);

#endif // APP_SAADC_H