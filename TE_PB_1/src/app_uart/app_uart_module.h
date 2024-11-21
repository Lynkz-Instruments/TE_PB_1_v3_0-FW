/**
 * @file app_uart_module.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef APP_UART_MODULE_H
#define APP_UART_MODULE_H

#include <stdint.h>
#include "sdk_errors.h"

/**
 * @brief Function to initialize UART communication between the Anna module and the USB interface.
 * 
 */
ret_code_t app_uart_init_PB(void);

/**
 * @brief Function to write on UART TX buffer.
 *
 */
ret_code_t app_uart_module_write(const uint8_t * p_data, const uint32_t size, uint32_t timeout_ms);

/**
 * @brief Function to read UART RX buffer.
 *
 */
ret_code_t app_uart_module_read(uint8_t * p_data, uint32_t size, uint32_t timeout_ms);

/**
 * @brief Function to flush UART RX buffer.
 *
 */
ret_code_t app_uart_module_flush(uint32_t timeout_ms);

/**
 * @brief Function to uninit UART module.
 *
 */
ret_code_t app_uart_module_uninit(void);

/**
 * @brief Function to set UART RX handling function.
 *
 */
void app_uart_module_set_rx_callback(void(*rx_handler)(const char c));


#endif // APP_UART_MODULE_H
