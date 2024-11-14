/**
* @author Xavier Bernard (xavier@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/

#ifndef APP_FLASH_H
#define APP_FLASH_H

#include <stdbool.h>
#include <stdint.h>

#include "app.h"

#define FILE_RECORD_COUNT              100                          // Number of sessions max to store in a file.
#define FFT_FILE_RECORD_COUNT          50                           // Number of fft max to store in a file.

#define FS_READ_SIZE                   32                           // Bytes
#define FS_PROG_SIZE                   FS_READ_SIZE                 // Bytes
#define FS_CACHE_SIZE                  32                           // Bytes
#define FS_LOOK_SIZE                   32                           // Bytes
#define FS_BLOCK_CYCLE                 100

#define FS_BLOCK_OFFSET                48                           // Block offset to start the file system.
#define FS_BLOCK_COUNT                 1024 - FS_BLOCK_OFFSET       // Number of blocks used for littlefs.
#define FS_BLOCK_SIZE                  4096                         // Bytes - Sector size for NOR Flash.

#define FFT_ANALYZER_ADDR              0x00000000                   // FFT data starting address in NOR Flash.
#define FFT_SIZE                       4096                         // Bytes - FFT result size in bytes = 2048 FFT bins.
#define FFT_FLASH_WRITE_SIZE           32                           // Bytes
#define FFT_DATA_SIZE_BYTES            sizeof(uint16_t)             // Bytes
#define FFT_TOTAL_RECORD               (FS_BLOCK_OFFSET * FFT_SIZE) // Bytes - Max data in bytes needed for an FFT. 

/**
 * @brief Function to import device config from the UICR.
 * 
 */
void app_flash_import_config(void);

/**
 * @brief Function to set the default device config in the UICR.
 * 
 */
void app_flash_set_default_config(void);

/**
 * @brief Function to save configuration in the UICR.
 * 
 */
bool app_flash_save_config(void);

/**
 * @brief Function to enable the flash.
 * 
 */
bool app_flash_enable(void);

/**
 * @brief Function to disable the flash.
 * 
 */
void app_flash_disable(void);

/**
 * @brief Function to test the flash.
 * 
 */
bool app_flash_test(void);

/**
 * @brief Function to create a new record session.
 * 
 */
bool app_flash_create_data_session(uint16_t * session_count);

/**
 * @brief Function to record an app packet.
 * 
 * @param data Pointer to store the app packet to write.
 */
bool app_flash_record_data_packet(uint8_t * data, uint32_t size);

/**
 * @brief Function to close a record session.
 * 
 */
bool app_flash_close_data_session(void);

/**
 * @brief Get the number of sessions recorded.
 * @return uint16_t -> Number of sessions.
 */
uint16_t app_flash_get_data_session_count(void);

/**
 * @brief Function to start the download of a given file id.
 *
 */
bool app_flash_download_data_file_start(uint8_t file_id, uint32_t * data_count);

/**
 * @brief Function to download a chunk of data of a specific size at a specific index.
 * 
 */
bool app_flash_download_data(uint32_t index, uint8_t * data, uint32_t size);

/**
 * @brief Function to stop the download of the current file.
 * 
 */
bool app_flash_download_data_file_stop(void);

/**
 * @brief Function to remove all data session files.
 * 
 */
bool app_flash_remove_data_sessions(void);

/**
 * @brief Function to create an FFT session.
 * 
 */
bool app_flash_create_fft_session(uint16_t * session_count, struct app_fft_header_t header);

/**
 * @brief Function to record an FFT packet.
 * 
 */
bool app_flash_record_fft_packet(uint8_t * data, uint32_t size);

/**
 * @brief Function to close the current FFT session.
 * 
 */
bool app_flash_close_fft_session(void);

/**
 * @brief Function to get FFT count.
 * 
 */
uint16_t app_flash_get_fft_session_count(void);

/**
 * @brief Function to start the download of a given file id.
 *
 */
bool app_flash_download_fft_file_start(uint8_t file_id, uint32_t * data_count);

/**
 * @brief Function to download a chunk of fft of a specific size at a specific index.
 * 
 */
bool app_flash_download_fft(uint32_t index, uint8_t * data, uint32_t size);

/**
 * @brief Function to stop the download of the current file.
 * 
 */
bool app_flash_download_fft_file_stop(void);

/**
 * @brief Function to remove all fft session files.
 * 
 */
bool app_flash_remove_fft_sessions(void);

/**
 * @brief Function to remove the fft data file.
 * 
 */
bool app_flash_remove_fft_data(void);

/**
 * @brief Function to append vibration data to the file.
 * 
 */
bool app_flash_append_vibration_data(uint16_t data);

/**
 * @brief Function to get vibration data file size.
 * 
 */
uint32_t app_flash_get_vibration_data_size(void);

/**
 * @brief Function to get vibration data from file.
 * 
 */
bool app_flash_get_vibration_data(uint8_t * data, uint8_t size);

/**
 * @brief Function to remove vibration data file.
 * 
 */
bool app_flash_remove_vibration_data(void);

/**
 * @brief Function to erase all the sector in NOR flash.
 * 
 */
bool app_flash_erase_all(void);

/**
 * @brief Function to get flash usage percentage.
 * 
 */
uint8_t app_flash_get_percentage(void);

/**
 * @brief Function to set the erasing flag.
 * 
 */
void app_flash_set_erasing(bool value);

/**
 * @brief Function to set the recording flag.
 * 
 */
void app_flash_set_recording(bool value);

/**
 * @brief Function to set the reading flag.
 * 
 */
void app_flash_set_reading(bool value);

/**
 * @brief Function to know if the flash is being erased. 
 * 
 * @return true -> Erasing
 * @return false -> Not erasing
 */
bool is_app_flash_erasing(void);

/**
 * @brief Function to know if the flash is recording.
 * 
 * @return true -> Recording
 * @return false -> Not recording
 */
bool is_app_flash_recording(void);

/**
 * @brief Function to know if we're reading the flash.
 * 
 * @return true -> Reading
 * @return false -> Not reading
 */
bool is_app_flash_reading(void);

#endif // APP_FLASH_H
