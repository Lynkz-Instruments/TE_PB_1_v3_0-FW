/**
 * @file app_uarte.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef APP_UARTE_H
#define APP_UARTE_H

#include <stdint.h>
#include "sdk_errors.h"
// #include "nrf_libuarte_drv.h"
#include "nrf_libuarte_async.h"

ret_code_t app_uarte_init_lora();
void app_uarte_uninit(void);

void app_uarte_set_rx_callback(void(*rx_handler)(const char c));
void app_uarte_set_buffer(uint8_t * buffer);

nrf_libuarte_async_t const * app_uarte_get_instance(void);

#endif // APP_UARTE_H