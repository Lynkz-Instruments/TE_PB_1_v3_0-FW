/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/
#ifndef APP_IMU_H
#define APP_IMU_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "app_hardware.h"

#include "bmi270.h"
#include "bmi270_nrf5.h"
#include "app.h"

extern bool is_imu_new_data;
extern uint8_t bmi_new_data_counter;

// Enum that represent the accelerometer range config.
enum APP_IMU_ACC_CONFIG_VALUE
{
  APP_IMU_ACC_2G  = 0x01,
  APP_IMU_ACC_4G  = 0x02,
  APP_IMU_ACC_8G  = 0x03,
  APP_IMU_ACC_16G = 0x04
};
typedef enum APP_IMU_ACC_CONFIG_VALUE app_imu_acc_config_value_t; 

// Enum that represent the gyroscope range config.
enum APP_IMU_GYRO_CONFIG_VALUE
{
  APP_IMU_GYRO_125DPS   = 0x01,
  APP_IMU_GYRO_250DPS   = 0x02,
  APP_IMU_GYRO_500DPS   = 0x03,
  APP_IMU_GYRO_1000DPS  = 0x04,
  APP_IMU_GYRO_2000DPS  = 0x05
};
typedef enum APP_IMU_GYRO_CONFIG_VALUE app_imu_gyro_config_value_t; 

// Enum that represent the BMI frequency config.
enum APP_IMU_FREQ_CONFIG_VALUE
{
  APP_IMU_FREQ_25HZ   = 0x01,
  APP_IMU_FREQ_50HZ   = 0x02,
  APP_IMU_FREQ_100HZ  = 0x03,
  APP_IMU_FREQ_200HZ  = 0x04,
  APP_IMU_FREQ_400HZ  = 0x05,
  APP_IMU_FREQ_800HZ  = 0x06,
  APP_IMU_FREQ_1600HZ = 0x07
};
typedef enum APP_IMU_FREQ_CONFIG_VALUE app_imu_freq_config_value_t; 

/**
 * @brief Function to initialize BMI.
 * @return bool
 */
bool app_imu_init(bool fft);

/**
 * @brief Function to uninitialize BMI.
 * @return bool
 */
void app_imu_uninit(void);

/**
 * @brief Get RMS accel and gyro on all axis
 * @return void 
 */
void app_imu_get_accel_gyro_rms(struct accelerometer_sensor_data_t * accel_data, struct gyroscope_sensor_data_t * gyro_data);

/**
 * @brief Get acceleration and gyroscope.
 *
 * @return bool 
 */
bool app_imu_read_accel_gyro(struct accelerometer_sensor_data_t * accel_data, struct gyroscope_sensor_data_t * gyro_data);

/**
 * @brief Get RMS module of acceleration.
 *
 * @return int16_t 
 */
int16_t app_imu_get_accel_module_rms(void);

/**
 * @brief Get RMS module of gyroscope. 
 *
 * @return int16_t 
 */
int16_t app_imu_get_gyro_module_rms(void);

#endif // APP_IMU_H
