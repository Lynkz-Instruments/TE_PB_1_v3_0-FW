/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/

#ifndef APP_PERIPHERALS
#define APP_PERIPHERALS

#include "lynkz_utils.h"

#include "app_ble.h"
#include "app_tasks.h"
#include "app_flash.h"
#include "app_imu.h"
#include "app_antenna.h"
#include "app_lora.h"

#include "mx25r.h"
#include "bmi270_nrf5.h"
#include "ble_nrf.h"
#include "app.h"

/**
 * @brief Structure to store data from test
 */
struct app_test_data_t{
  // FLASH
  bool flash_ok;

  // LoRa
  bool lora_ok;
  char lora_deveui[24];

  // BLE
  bool ble_ok;
  uint8_t ble_device_addr[7];
  
  // BMI
  bool bmi_ok;
  int16_t accel_x;       // Acceleration in the X axis
  int16_t accel_y;       // Acceleration in the Y axis
  int16_t accel_z;       // Acceleration in the Z axis
  int16_t gyro_x;        // Gyroscope drift in the X axis
  int16_t gyro_y;        // Gyroscope drift in the Y axis
  int16_t gyro_z;        // Gyroscope drift in the Z axis

  // Sensors
  bool tsys02d_ok;       // Temperature sensor OK
  bool ldc1614_ok;       // Resonant Frequency Sensor OK
  int16_t temp;          // Temperature
  uint32_t channel_0;    // Resonant Frequency for channel 0
  uint32_t channel_1;    // Resonant Frequency for channel 1
  
  // Interfaces
  bool i2c_ok;
  bool spi_ok;
  bool uart_ok;
  bool nfc_ok;

}__attribute__((packed));

/**
 * @brief Function to setup all peripherals :  
 * (rtc : pcf85063, nor flash : mxr25, humidity/temperature : bme280, accel/gyro :bmi270)
 * The function also serves as the built-in self-test for the TAG
 * 
 */
bool app_peripherals_self_test(struct app_test_data_t * test_data);


#endif // APP_PERIPHERALS