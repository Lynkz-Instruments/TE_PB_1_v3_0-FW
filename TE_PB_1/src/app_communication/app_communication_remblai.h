/**
 * @file app_communication.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef APP_COMMUNICATION_REMBLAI_H
#define APP_COMMUNICATION_REMBLAI_H

#include <stdint.h>
#include "nrf.h"

/**
 * @brief Callback to handle a NUS received event.
 * 
 * @param data Data received
 * @param len
 */
void app_comm_remblai_process(uint8_t const* data, uint16_t len);

# endif // APP_COMMUNICATION_REMBLAI_H