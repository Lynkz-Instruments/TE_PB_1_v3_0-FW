 /* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#ifndef SIMPLE_UART_H
#define SIMPLE_UART_H

/*lint ++flb "Enter library region" */

#include <stdbool.h>
#include <stdint.h>

/** @file
* @brief Simple UART driver
*
*
* @defgroup nrf_drivers_simple_uart Simple UART driver
* @{
* @ingroup nrf_drivers
* @brief Simple SW UART driver
*/

typedef enum
{
    UART_BAUD_9600      = 1667,
    UART_BAUD_19200     = 833,
    UART_BAUD_38400     = 417,
    UART_BAUD_57600     = 278,
    UART_BAUD_115200    = 139
} simple_sw_baud_rate_t;

bool simple_uart_put(uint8_t cr);

void simple_uart_putstring(const uint8_t *str);

void simple_sw_uart_config(uint8_t txd_pin_number, simple_sw_baud_rate_t baud_rate);

#endif
