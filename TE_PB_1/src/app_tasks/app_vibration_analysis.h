/**
 * @file app_vibration_analysis.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief
 * @version 1.0
 * @date 2024-02-06

 * @copyright Copyright (c) Lynkz Instruments Inc. Amos 2023
 */

#ifndef APP_VIBRATION_ANALYSIS_H
#define APP_VIBRATION_ANALYSIS_H

#include <stdbool.h>
#include <stdint.h>

#include "app.h"

extern int16_t fft_gain;
extern uint8_t fft_freq;

/**
 * @brief Function to perform an FFT and store in NOR Flash.
 * 
 */
void app_vibration_fft(uint16_t* buf_out, const uint16_t buffer_size);

/**
 * @brief Function to compute the acceleration RMS.
 * 
 */
uint16_t app_vibration_analyze(void);

#endif // APP_VIBRATION_ANALYSIS_H