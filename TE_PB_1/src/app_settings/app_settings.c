/**
 * @file app_settings.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 0.1
 * @date 2023-04-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "app_settings.h"

#include <string.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf5_utils.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define APP_SETTINGS_VERBOSE 2

static struct app_config_t device_config;
static char m_deveui[17] = {0};
static char m_dev_addr[9] = {0};
static char m_apps_key[33] = {0};
static char m_nets_key[33] = {0};

void app_settings_show_config(void)
{
  // Showing information for debug
  #if APP_SETTINGS_VERBOSE >= 2
  NRF_LOG_INFO("Smart Liner device configurations:");
  NRF_LOG_INFO("LoRa heart beat period (min): %d", app_settings_get_lora_heartbeat_period_minutes());
  NRF_LOG_INFO("Record duration (sec): %d", app_settings_get_record_duration_seconds());
  NRF_LOG_INFO("Record period (min): %d", app_settings_get_record_period_minutes());
  NRF_LOG_INFO("FFT period (hr): %d", app_settings_get_fft_period_hours());
  NRF_LOG_INFO("Accel range (preset): %d", app_settings_get_accelerometer_range());
  NRF_LOG_INFO("Gyro range (preset): %d", app_settings_get_gyroscope_range());
  NRF_LOG_INFO("IMU Frequency (preset): %d", app_settings_get_imu_frequency());
  NRF_LOG_INFO("Channel 0 drive current: %d", app_settings_get_ch0_drive_current());
  NRF_LOG_INFO("Channel 1 drive current: %d", app_settings_get_ch1_drive_current());
  NRF_LOG_INFO("Channel 0 settle count: %d", app_settings_get_ch0_settle_count());
  NRF_LOG_INFO("Channel 1 settle count: %d", app_settings_get_ch1_settle_count());
  NRF_LOG_INFO("Channel enabled bitmask: %d", app_settings_get_ch_enabled_bitmask());
  #endif
}

bool app_settings_set_configuration(uint8_t * data)
{
  if (data){
    struct app_config_t * config = (struct app_config_t *)data;
    
    if (config->lora_heartbeat_period_minutes == 0xFF || config->lora_heartbeat_period_minutes == 0x00){
      config->lora_heartbeat_period_minutes = LORA_HB_PERIOD_MINUTES_DEFAULT;
    }
    if (config->record_duration_seconds == 0xFFFF || config->record_duration_seconds == 0x0000 || config->record_duration_seconds < 5 || config->record_duration_seconds > 300){
      config->record_duration_seconds = RECORD_DURATION_SECONDS_DEFAULT;
    }
    if (config->record_period_minutes == 0xFFFF || config->record_period_minutes == 0x0000){
      config->record_period_minutes = RECORD_PERIOD_MINUTES_DEFAULT;
    }
    if (config->fft_period_hours == 0xFF){
      config->fft_period_hours = FFT_PERIOD_HOURS_DEFAULT;
    }
    if (config->accelerometer_range == 0xFF || config->accelerometer_range == 0x00 || config->accelerometer_range > APP_IMU_ACC_16G || config->accelerometer_range < APP_IMU_ACC_2G){
      config->accelerometer_range = ACCELEROMETER_RANGE_DEFAULT;
    }
    if (config->gyroscope_range == 0xFF || config->gyroscope_range == 0x00 || config->gyroscope_range > APP_IMU_GYRO_2000DPS || config->gyroscope_range < APP_IMU_GYRO_125DPS){
      config->gyroscope_range = GYROSCOPE_RANGE_DEFAULT;
    }
    if (config->imu_frequency == 0xFF || config->imu_frequency == 0x00 || config->imu_frequency > APP_IMU_FREQ_1600HZ || config->imu_frequency < APP_IMU_FREQ_25HZ){
      config->imu_frequency = IMU_FREQUENCY_DEFAULT;
    }
    if (config->ch0_drive_current == 0xFF || config->ch0_drive_current == 0x00 || config->ch0_drive_current > 0b11111) {
      config->ch0_drive_current = CH0_DRIVE_CURRENT_DEFAULT;
    }
    if (config->ch1_drive_current == 0xFF || config->ch1_drive_current == 0x00 || config->ch1_drive_current > 0b11111) {
      config->ch1_drive_current = CH1_DRIVE_CURRENT_DEFAULT;
    }
    if (config->ch0_settle_count == 0xFFFF || config->ch0_settle_count == 0x0000) {
      config->ch0_settle_count = CH0_SETTLE_COUNT_DEFAULT;
    }
    if (config->ch1_settle_count == 0xFFFF || config->ch1_settle_count == 0x0000) {
      config->ch1_settle_count = CH1_SETTLE_COUNT_DEFAULT;
    }
    if (config->ch_enabled_bitmask == 0xFF || config->ch_enabled_bitmask == 00) {
      config->ch_enabled_bitmask = CH_ENABLED_BITMASK_DEFAULT;
    }

    // Record period too short
    if ((uint16_t)ceilf(config->record_duration_seconds / 60.0f) >= config->record_period_minutes){
      // At least 5 minutes after the session recording is done.
      config->record_period_minutes = (uint16_t)ceilf(config->record_duration_seconds / 60.0f) + 5;
    }

    device_config = *config;
  }
  else{
    return false;
  }
  return true;
}

void app_settings_set_lora_heartbeat_period_minutes(uint8_t value)
{
  device_config.lora_heartbeat_period_minutes = value;
}

void app_settings_set_record_duration_seconds(uint16_t value)
{
  device_config.record_duration_seconds = value;
}

void app_settings_set_record_period_minutes(uint16_t value)
{
  device_config.record_period_minutes = value;
}

bool app_settings_get_configuration(uint8_t * data)
{
  if (data){
    memcpy(data, &device_config, sizeof(struct app_config_t));
    return true;
  }
  return false;
}

uint8_t app_settings_get_lora_heartbeat_period_minutes(void)
{
  return device_config.lora_heartbeat_period_minutes;
}

uint16_t app_settings_get_record_duration_seconds(void)
{
  return device_config.record_duration_seconds;
}

uint16_t app_settings_get_record_period_minutes(void)
{
  return device_config.record_period_minutes;
}

uint8_t app_settings_get_fft_period_hours(void)
{
  return device_config.fft_period_hours;
}

uint8_t app_settings_get_accelerometer_range(void)
{
  return device_config.accelerometer_range;
}

uint8_t app_settings_get_gyroscope_range(void)
{
  return device_config.gyroscope_range;
}

uint8_t app_settings_get_imu_frequency(void)
{
  return device_config.imu_frequency;
}

uint8_t app_settings_get_ch0_drive_current(void)
{
  return device_config.ch0_drive_current;
}

uint8_t app_settings_get_ch1_drive_current(void)
{
  return device_config.ch1_drive_current;
}

uint16_t app_settings_get_ch0_settle_count(void)
{
  return device_config.ch0_settle_count;
}

uint16_t app_settings_get_ch1_settle_count(void)
{
  return device_config.ch1_settle_count;
}

uint8_t app_settings_get_ch_enabled_bitmask(void)
{
  return device_config.ch_enabled_bitmask;
}

void app_settings_set_lora_keys(char * deveui, char * dev_addr, char * apps_key, char * nets_key)
{
  memcpy(m_deveui, deveui, 17);
  memcpy(m_dev_addr, dev_addr, 9);
  memcpy(m_apps_key, apps_key, 33);
  memcpy(m_nets_key, nets_key, 33);
}

void app_settings_get_lora_keys(char * deveui, char * dev_addr, char * apps_key, char * nets_key)
{
  memcpy(deveui, m_deveui, 17);
  memcpy(dev_addr, m_dev_addr, 9);
  memcpy(apps_key, m_apps_key, 33);
  memcpy(nets_key, m_nets_key, 33);
}
