/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) Lynkz Instruments Inc, Amos 2024
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

typedef enum{
STCO_ERROR = 0,
STCO_OK = 1,
STCO_OPEN_Z = 2,
STCO_SHORT_Z = 3,
} STARTCO_t;

#define WAKE_UP_TIME_SEC 0.1 // Wake up time in seconds

/**
 * @brief Main function to initialize all the hardware modules.
 * 
 * @return true -> Initilization done
 * @return false -> Initilization failed
 */
bool app_hdw_init(void);

/**
 * @brief Function to kick the watchdog.
 * 
 */
void app_hdw_wdt_kick(void);

/**
 * @brief Function to configure the board for certain modes.
 * 
 */
void app_hdw_select_mode();

/**
 * @brief Function to select the device to communicate with.
 * 
 */
void app_hdw_select_UART();

/**
 * @brief Function to set the 1st analog switch.
 * 
 */
void app_hdw_set_analog_switch1(bool on);

/**
 * @brief Function to set the 2nd analog switch.
 * 
 */
void app_hdw_set_analog_switch2(bool on);

/**
 * @brief Function to set the 3rd analog switch.
 * 
 */
void app_hdw_set_analog_switch3(bool on);

/**
 * @brief Function to set the 4th and the 5th analog switch.
 * 
 */
void app_hdw_set_analog_switch4_5(bool on);

/**
 * @brief Function to set the 6th analog switch.
 * 
 */
void app_hdw_set_analog_switch6(bool on);

/**
 * @brief Function to set the internal STARTCO led.
 * 
 */
void app_hdw_set_INT_STCO_led(bool on);

/**
 * @brief Function to set the Bavard led.
 * 
 */
void app_hdw_set_INT_BV_led(bool on);

/**
 * @brief Function to set the 1st uart multiplexer led.
 * 
 */
void app_hdw_set_UART1_led(bool on);

/**
 * @brief Function to set the 2nd uart multiplexer led.
 * 
 */
void app_hdw_set_UART2_led(bool on);

/**
 * @brief Function to set the low battery power led.
 * 
 */
void app_hdw_set_low_bat_led(bool on);

/**
 * @brief Function to set the power output to power the TAG.
 * 
 */
void app_hdw_set_TAG_pwr(bool on);

/**
 * @brief Function to read the mode button and change the mode value.
 * 
 */
void app_hdw_read_mode_BTN();

/**
 * @brief Function to read the UART button and change the UART device.
 * 
 */
void app_hdw_read_UART_BTN();

/**
 * @brief Function to indicate the battery's state.
 * 
 */
void app_hdw_read_V_BAT();

/**
 * @brief Function to read the STARTCO state
 * 
 */
STARTCO_t app_hdw_read_STCO();


/**
 * @brief Function to detect a TAG's presence and ensure it is powered.
 * 
 */
void app_hdw_detect_TAG();

void app_hdw_set_uart_ble(bool enable);

#endif // APP_HARDWARE_H