/**
 * @file app_lora.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef APP_LORA_H
#define APP_LORA_H

#include <stdint.h>
#include <stdbool.h>
#include "sdk_errors.h"

// Keys and ID for LoRa
#define APP_LORA_APPKEY "82:ad:29:b2:69:15:cd:a1:2e:3a:96:7c:c0:56:8d:92"
#define APP_LORA_APPEUI "70:b3:d5:7e:d0:03:54:86"

/**
 * @brief Function to initialize LoRa radio.
 * 
 */
ret_code_t app_lora_init(bool uartInit);

/**
 * @brief Function to wakeup LoRa radio.
 * 
 */
ret_code_t app_lora_wakeup(void);

/**
 * @brief Function to sleep LoRa radio.
 * 
 */
ret_code_t app_lora_sleep(void);

/**
 * @brief Function to get deveui from LoRa radio.
 * 
 */
ret_code_t app_lora_getdeveui(char * deveui);

/**
 * @brief Function to init UART for LoRa radio.
 * 
 */
ret_code_t app_lora_init_uart(void);

/**
 * @brief Function to uninit UART for LoRa radio.
 * 
 */
ret_code_t app_lora_uninit_uart(void);

/**
 * @brief Function send a start packet with LoRa radio.
 * 
 */
void app_lora_send_start(void);

/**
 * @brief Function send firmware version with LoRa radio.
 * 
 */
void app_lora_send_version(void);

/**
 * @brief Function send an heart beat with LoRa radio.
 * 
 */
void app_lora_send_heartbeat(uint8_t * data, uint8_t len, bool check_downlink);

/**
 * @brief Function send a data packet with LoRa radio.
 * 
 */
void app_lora_send_data_pkt(uint8_t * data, uint8_t len, bool check_downlink);

/**
 * @brief Function send a vibration data packet with LoRa radio.
 * 
 */
void app_lora_send_vibration_data_pkt(uint16_t period, uint8_t * data, uint8_t len, bool check_downlink);

/**
 * @brief Function send an fft packet with LoRa radio.
 * 
 */
void app_lora_send_fft_pkt(uint16_t fft_id, uint8_t chunk_id, int16_t gain, uint8_t freq, uint8_t * data, uint8_t len, bool check_downlink);

/**
 * @brief Function joined state of LoRa radio.
 * 
 */
bool app_lora_joined(void);

#endif // APP_LORA_H
