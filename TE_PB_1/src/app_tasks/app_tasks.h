/**
 * @file app_tasks.h
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief 
 * @version 1.0
 * @date 2023-04-15
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "app_peripherals.h"
#include "app_ble.h"

void app_task_set_advertising(bool val);
bool app_task_is_advertising(void);

/**
 * @brief Function to setup the main periodic tasks. 
 */
void setup_tasks(void);

/**
 * @brief Function to start the saving configuration task. 
 */
void app_tasks_save_config(void);

/**
 * @brief Function to start the data sessions BLE download. 
 */
void app_tasks_data_ble_download(void);

/**
 * @brief Function to start the FFT sessions BLE download. 
 */
void app_tasks_fft_ble_download(void);

/**
 * @brief Function to perform an FFT. 
 */
void app_tasks_perform_fft(void);

/**
 * @brief Function to get the number of data sessions. 
 */
void app_tasks_get_session_count(void);

/**
 * @brief Function to get the number of FFT sessions. 
 */
void app_tasks_get_fft_count(void);

/**
 * @brief Function to erase the whole NOR flash. 
 */
void app_tasks_erase_all(void);

/**
 * @brief Function to erase data files. 
 */
void app_tasks_erase_data(void);

/**
 * @brief Function to erase FFT files. 
 */
void app_tasks_erase_fft(void);

/**
 * @brief Function to power off the device. 
 */
void app_tasks_power_off_device(void);

/**
 * @brief Function to request data from the device. 
 */
void app_tasks_request_data(void);

#endif // APP_TASKS_H