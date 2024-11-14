/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/
#ifndef APP_ANTENNA_H
#define APP_ANTENNA_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize the antenna assembly.
 * 
 * @param[in] bitmask   Enable channel bitmask (0x01 = ch0, 0x02 = ch1, 0x03 = ch1 and ch2)
 * @return    bool
 */
bool app_antenna_init(uint8_t bitmask);

/**
 * @brief Sleep the antenna assembly to save current.
 * 
 * @return bool
 */
bool app_antenna_sleep(void);

/**
 * @brief Uninitialize the antenna assembly.
 * 
 * @return void
 */
void app_antenna_uninit(void);

/**
 * @brief Wake-up the antenna assembly from sleep.
 * 
 * @return bool
 */
bool app_antenna_wake_up(void);

/**
 * @brief      Get antenna frequency from selected channel.
 *
 * @param[in]  channel     Channel to get the frequency from
 * @param[out] value       Frequency code
 * @param[out] error_mask  Error mask for LoRa uplink
 *
 * @return     bool
 */
bool app_antenna_get_frequency(uint8_t channel, uint32_t* value, uint8_t* error_mask);

/**
 * @brief Get temperature from antenna.
 * 
 * @return void
 */
bool app_antenna_get_temperature(uint16_t* temperature);

#endif // APP_ANTENNA_H
