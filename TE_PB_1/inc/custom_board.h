/**
* @author Xavier Bernard (xavier@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/
#ifndef BOARD_CUSTOM_H
#define BOARD_CUSTOM_H
#ifdef __cplusplus
extern "C" {
#endif

//LEDS Default Pin Definition
#define LEDS_NUMBER 1

#define BLUE_LED 25 // Blue Status LED
#define GREEN_LED 26 // Green Status LED
#define RED_LED 27 // Red Status LED

//UART Default Pin Definition
#define UART_RX_PIN_NUMBER 14
#define UART_TX_PIN_NUMBER 15
#define TB_RX_PIN_NUMBER 18
#define TB_TX_PIN_NUMBER 16
#define SERIAL_RTS_PIN 0xFF
#define SERIAL_CTS_PIN 0xFF

//SPI Default Pin Definition
#define SPIM1_SCK_PIN         11  // SPI clock GPIO pin number.
#define SPIM1_MOSI_PIN        4   // SPI Master Out Slave In GPIO pin number.
#define SPIM1_MISO_PIN        5   // SPI Master In Slave Out GPIO pin number.
#define SPIM1_CSB_FLASH_PIN   3   // Flash SPI Slave Select GPIO pin number.
#define SPIM1_CSB_IMU_PIN     19  // IMU SPI Slave Select GPIO pin number.

//I2C Default Pin Definition
#define I2CM0_SDA_PIN   20   // I2C SDA Pin Number.
#define I2CM0_SCL_PIN   24   // I2C SCL Pin Number.

//Interrupt Pin Definition
#define IMU_INT_PIN 29

// Peripheral PWR GPIO
#define PWR_FLASH_BMI_PIN 2
// #define PWR_IMU_PIN 31
#define PWR_LORA_PIN 28
#define PWR_ANTENNA_PIN 22

// Test Board Detect GPIO
#define TB_DETECT_PIN 30

// LoRa Reset GPIO
#define LORA_RST_PIN 23

// NFC
#define NFC1_PIN 9
#define NFC2_PIN 10

// Low frequency clock source to be used by the SoftDevice
#define NRF_CLOCK_LFCLKSRC {.source = NRF_CLOCK_LF_SRC_XTAL, \
.rc_ctiv = 0, \
.rc_temp_ctiv = 0, \
.xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}
#ifdef __cplusplus
}
#endif

#endif // BOARD_CUSTOM_H