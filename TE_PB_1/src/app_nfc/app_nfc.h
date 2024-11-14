#ifndef APP_NFC_H
#define APP_NFC_H

// This module require the module app_uicr

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"

#include <sdk_errors.h>

#include "nrfx_nfct.h"
#include "nfc_t4t_lib.h"
#include "nfc_ndef_msg.h"

#include "nrf5_utils.h"

#define APP_NFC_T4T_BUFFER_LEN 16

/**
 * @brief Main function to initialize all the hardware modules.
 */
void app_nfc_init(void);

/**
 * @brief Setup the nfc in wake up on field detect mode.
 * 
 * @return NRF_SUCCES -> Succes
 * @return NRF_ERRORx -> Error
 */
ret_code_t app_nfc_wake_up_mode(void);

/**
 * @brief Setup the nfc in type 4 tag emulation mode.
 * 
 * @return NRF_SUCCES -> Succes
 * @return NRF_ERRORx -> Error
 */
ret_code_t app_nfc_t4t_mode(void);

/**
 * @brief Get the data write in the type 4 tag
 *
 * @return the data size
 */
uint16_t app_nfc_t4t_get_data(uint8_t* buffer_p, uint16_t max_size);

/**
 * @brief Write data in the type 4 tag
 *
 */
void app_nfc_t4t_set_data(const uint8_t* const buffer_p, uint16_t size);

/**
 * @brief Get the size of the data write in the type 4 tag
 *
 * @return the data size
 */
uint16_t app_nfc_t4t_get_size(void);

/**
 * @brief Get the field detected value
 *
 * @return true if a event occur since the last call off this function
 */
bool app_nfc_get_and_clear_event_flag(void);

/**
 * @brief Get the size of the data write in the type 4 tag
 *
 * @return the data size
 */
void app_nfc_t4t_set_handler( void(handler)(const uint8_t *p_data, size_t data_length) );

#endif //APP_NFC_H