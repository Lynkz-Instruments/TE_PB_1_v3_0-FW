/**
 * @file app.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-01-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include <stdint.h>

// BLE
// 0 -> No
// 1 -> Yes
#define APP_BLE 1

// LORA
// 0 -> No
// 1 -> Yes
#define APP_LORA 1

#define PACKET_CRC_POLYNOMIAL 0x12

#define BLE_DEVICE_NAME_MAX_SIZE 26 // Characters

// There are 32 UICR 32 bits registers
// UICR config
#define UICR_PANEL_NO_LSB_ID  1
#define UICR_PANEL_NO_MSB_ID  2
#define UICR_PCBA_NO_ID       3
#define UICR_HWVER_MIN_ID     4
#define UICR_HWVER_MAJ_ID     5
#define UICR_BATCHNO_LSB_0_ID 6
#define UICR_BATCHNO_LSB_1_ID 7
#define UICR_BATCHNO_MSB_2_ID 8
#define UICR_BATCHNO_MSB_3_ID 9

// UICR config smart liner
#define UICR_LORA_HB_PERIOD_MINUTES_ID  11
#define UICR_RECORD_DURATION_SECONDS_ID 13
#define UICR_RECORD_PERIOD_MINUTES_ID   14
#define UICR_FFT_PERIOD_HOURS_ID        15
#define UICR_ACCELEROMETER_RANGE_ID     16
#define UICR_GYROSCOPE_RANGE_ID         17
#define UICR_IMU_FREQUENCY_ID           18
#define UICR_CH0_DRIVE_CURRENT_ID       19
#define UICR_CH1_DRIVE_CURRENT_ID       20
#define UICR_CH0_SETTLE_COUNT_ID        21
#define UICR_CH1_SETTLE_COUNT_ID        22
#define UICR_CH_ENABLED_BITMASK_ID      (23)

/**
 * @brief Structure to store the app configurations
 */
struct app_config_t
{
  uint8_t lora_heartbeat_period_minutes; // LoRa packet heart beat periods in minutes.
  uint16_t record_duration_seconds;      // Recording duration.
  uint16_t record_period_minutes;        // Recording period.
  uint8_t fft_period_hours;              // FFT period.
  uint8_t accelerometer_range;           // Accelerometer range.
  uint8_t gyroscope_range;               // Gyroscope range.
  uint8_t imu_frequency;                 // IMU frequency.
  uint8_t ch0_drive_current;             // Antenna channel 0 drive current
  uint8_t ch1_drive_current;             // Antenna channel 1 drive current
  uint16_t ch0_settle_count;             // Antenna channel 0 settle count
  uint16_t ch1_settle_count;             // Antenna channel 1 settle count
  uint8_t ch_enabled_bitmask;            // Channel enabled bitmask
}__attribute__((packed));

/**
 * @brief Structure to store the app version packet.
 */
struct app_version_packet_t
{
  uint8_t major; // Version major part 
  uint8_t minor; // Version minor part
  uint8_t patch; // Version patch part                                  
}__attribute__((packed));

/**
 * @brief Structure to store the app data packet.
 */
struct app_packet_t
{   
  uint16_t record_id;    // Bytes   0-1    Record id (0 to 65535)
  uint16_t temp;         // Bytes   2-3    Temperature (raw)
  uint16_t accel_mod;    // Bytes   4-5    Accel Module in 0.1mg
  uint16_t gyro_mod;     // Bytes   6-7    Gyro Module in deg./s
  uint32_t freq_chan_0;  // Bytes   8-11   Coil resonant frequency average (raw)
  uint32_t freq_chan_1;  // Bytes   12-15  Capacituve sensor resonant frequency average (raw)
  uint8_t err_chan_0;    // Byte    16     Error bitmask for channel 0
  uint8_t err_chan_1;    // Byte    17     Error bitmask for channel 1
}__attribute__((packed));

/**
 * @brief Structure to store the app fft packet.
 */
struct app_fft_header_t
{
  uint16_t fft_id; // FFT id (0 to 65535)
  int16_t gain;    // FFT computed gain
  uint8_t freq;    // FFT frequency
}__attribute__((packed));

#endif // APP_H
