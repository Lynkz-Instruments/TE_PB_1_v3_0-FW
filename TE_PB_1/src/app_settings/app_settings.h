/**
 * @file app_settings.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-04-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include "app.h"
#include "app_imu.h"

// Default config SmartLiner
// Set default configuration here
#define LORA_HB_PERIOD_MINUTES_DEFAULT  (15)
#define RECORD_DURATION_SECONDS_DEFAULT (20)
#define RECORD_PERIOD_MINUTES_DEFAULT   (60)
#define FFT_PERIOD_HOURS_DEFAULT        (0)
#define ACCELEROMETER_RANGE_DEFAULT     (APP_IMU_ACC_4G)
#define GYROSCOPE_RANGE_DEFAULT         (APP_IMU_GYRO_2000DPS)
#define IMU_FREQUENCY_DEFAULT           (APP_IMU_FREQ_1600HZ)
#define CH0_DRIVE_CURRENT_DEFAULT       (19)
#define CH1_DRIVE_CURRENT_DEFAULT       (22)
#define CH0_SETTLE_COUNT_DEFAULT        (0x0040)
#define CH1_SETTLE_COUNT_DEFAULT        (0x0200)
#define CH_ENABLED_BITMASK_DEFAULT      (0x01)

/**
 * @brief Function to show the device configurations.
 */
void app_settings_show_config(void);

/**
 * @brief Function to set the device configurations. 
 */
bool app_settings_set_configuration(uint8_t * data);

/**
 * @brief Function to set the LoRa heart beat period. 
 */
void app_settings_set_lora_heartbeat_period_minutes(uint8_t value);

/**
 * @brief Function to set the record duration. 
 */
void app_settings_set_record_duration_seconds(uint16_t value);

/**
 * @brief Function to set the record period. 
 */
void app_settings_set_record_period_minutes(uint16_t value);

/**
 * @brief Function to get the device configurations. 
 */
bool app_settings_get_configuration(uint8_t * data);

/**
 * @brief Function to get the LoRa heart beat period. 
 */
uint8_t app_settings_get_lora_heartbeat_period_minutes(void);

/**
 * @brief Function to get the record duration. 
 */
uint16_t app_settings_get_record_duration_seconds(void);

/**
 * @brief Function to get the record period. 
 */
uint16_t app_settings_get_record_period_minutes(void);

/**
 * @brief Function to get the FFT period. 
 */
uint8_t app_settings_get_fft_period_hours(void);

/**
 * @brief Function to get the accelerometer range. 
 */
uint8_t app_settings_get_accelerometer_range(void);

/**
 * @brief Function to get the gyroscope range. 
 */
uint8_t app_settings_get_gyroscope_range(void);

/**
 * @brief Function to get the BMI frequency. 
 */
uint8_t app_settings_get_imu_frequency(void);

/**
 * @brief Function to get the drive current for the channel 0.
 */
uint8_t app_settings_get_ch0_drive_current(void);

/**
 * @brief Function to get the drive current for the channel 1.
 */
uint8_t app_settings_get_ch1_drive_current(void);

/**
 * @brief Function to get the settle count for the channel 0.
 */
uint16_t app_settings_get_ch0_settle_count(void);

/**
 * @brief Function to get the settle count for the channel 1.
 */
uint16_t app_settings_get_ch1_settle_count(void);

/**
 * @brief Function to get the channel enabled bitmask.
 */
uint8_t app_settings_get_ch_enabled_bitmask(void);

/**
 * @brief Function to set the LoRa keys. 
 */
void app_settings_set_lora_keys(char * deveui, char * dev_addr, char * apps_key, char * nets_key);

/**
 * @brief Function to get the LoRa keys. 
 */
void app_settings_get_lora_keys(char * deveui, char * dev_addr, char * apps_key, char * nets_key);

#endif // APP_SETTINGS_H
