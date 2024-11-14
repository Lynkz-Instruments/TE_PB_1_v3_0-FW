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

#ifndef APP_COMMUNICATION_H
#define APP_COMMUNICATION_H

#include <stdint.h>
#include "nrf.h" 

/**
 * @brief Function to process the downlinks received from LoRa
 * 
 * @param data pointer
 * @param len
 */
void app_comm_lora_process(uint8_t * data, uint8_t len);

/**
 * @brief Callback to handle a NUS received event.
 * 
 * @param data Data received
 * @param len
 */
void app_comm_process(uint8_t const* data, uint16_t len);

/**
 * @brief Function to send a packet over NUS service.
 * 
 * @param data Pointer to the data to send.
 * @param len Data length
 */
void app_comm_send_packet(uint8_t * data, uint8_t len);

/**
 * @brief Function to send an acknowledge packet. Essentialy a "ok" message.
 */
void app_comm_send_ack(void);

/**
 * @brief Function to send a response packet. Essentialy a "done" message.
 */
void app_comm_send_response(void);

/**
 * @brief Function to send a failed message. Essentialy an "failed" message.
 */
void app_comm_send_fail(void);

/**
 * @brief Function to send an empty message. Essentialy an "empty" message.
 */
void app_comm_send_empty(void);

#endif // APP_COMMUNICATION_H