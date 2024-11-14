/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/

#ifndef APP_BLE_H
#define APP_BLE_H

#include "ble_nrf.h"
#include "app_hardware.h"

#define DEVICE_NAME   BLE_NAME_VER    /**< Name of device. Will be included in the advertising data. */

extern bool ble_nus_comm_ok;

/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send.
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
void nus_data_handler(ble_nus_evt_t * p_evt);

/**
 * @brief Function for printing tag debug to rtt viewer and to nus if char 'v' has been received over nus, setting tag in verbose mode.
 *
 * @details This function is variadic, it allows to print different value as : print_debug_tag("my value: %d", my_value)
 *          
 * @param *fmt string to print
 * @param ... variadic value to print in the string
 */
void print_debug_tag(const char *fmt, ...);

/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 * @param[in] p_context       Nordic UART Service context.
 */
void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);

/**@brief Function for initialising ble.
 *
 */
void app_ble_init(void);

#endif // APP_BLE_H