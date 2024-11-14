/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/

#ifndef APP_HARDWARE_H
#define APP_HARDWARE_H

// nRF5 SDK
#include "custom_board.h"
#include "boards.h"

#include "nrf_drv_wdt.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"
#include "nrf_drv_gpiote.h"

#include "nrf_gpio.h"
#include "nrf_delay.h"

// Lynkz Library
#include "ble_nrf.h"
#include "nrf5_utils.h"
#include "lynkz_utils.h"
#include "scheduler.h"


#define WAKE_UP_TIME_SEC 0.1 // Wake up time in seconds

/**
 * @brief Main function to initialize all the hardware modules.
 * 
 * @return true -> Initilization done
 * @return false -> Initilization failed
 */
bool app_hdw_init(void);

/**
 * @brief Function to set all the leds.
 * 
 */
void app_hdw_set_leds(bool red, bool green, bool blue);

/**
 * @brief Function to set red led.
 * 
 */
void app_hdw_set_red_led(bool on);

/**
 * @brief Function to set green led.
 * 
 */
void app_hdw_set_green_led(bool on);

/**
 * @brief Function to set blue led.
 * 
 */
void app_hdw_set_blue_led(bool on);

/**
 * @brief Function to power on/off LoRa radio.
 * 
 */
void app_hdw_pwr_lora(bool state);

/**
 * @brief Function to power on/off NOR flash and BMI.
 * 
 */
void app_hdw_pwr_flash_bmi(bool state);

/**
 * @brief Function to power on/off antenna assembly.
 * 
 */
void app_hdw_pwr_antenna(bool state);

/**
 * @brief Function to disconnect the LoRa UART pins.
 * 
 */
void app_hdw_disconnect_lora_uart(void);

/**
 * @brief Function to disconnect the SPI pins.
 * 
 */
void app_hdw_disconnect_spi(void);

/**
 * @brief Function to disconnect the I2C pins.
 * 
 */
void app_hdw_disconnect_i2c(void);

/**
 * @brief Function to set the GPIOs in low power mode.
 * 
 */
void app_hdw_gpio_low_power(void);

/**
 * @brief Function to know if the device is on a test board.
 * 
 */
bool app_hdw_is_on_test_board(void);

/**
 * @brief Function to disconnect the test board detect pin.
 * 
 */
void app_hdw_disconnect_test_board_detect(void);

/**
 * @brief Function to kick the watchdog.
 * 
 */
void app_hdw_wdt_kick(void);

#endif // APP_HARDWARE_H