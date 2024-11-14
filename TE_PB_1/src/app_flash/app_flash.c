/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/

#include "app_flash.h"

#include "app_settings.h"
#include "mx25r_nrf5.h"
#include "app_spi.h"

#include "lfs.h"

// 0 -> No log
// 1 -> Error only
// 2 -> Error and info
#define APP_FLASH_VERBOSE 1

#define CRC_INDEX 4

#define MAX_FILE_NAME 32 // Max number of char for a file name including folder.

#define MAX_DATA_FOLDER_SIZE 400 * 4096 // 400 blocks total for data
#define MAX_FFT_FOLDER_SIZE 400 * 4096  // 400 blocks total for FFT

// Flash model: MX25L3233F
// Doc: https://www.macronix.com/Lists/Datasheet/Attachments/8933/MX25L3233F,%203V,%2032Mb,%20v1.7.pdf

// Structure for the record_count file.
struct app_record_count_t
{
  uint16_t session_count;  // Session count
  uint16_t fft_count;      // FFT count                   
}__attribute__((packed));

static bool erasing = false;    // Erasing flag
static bool recording = false;  // Recording flag
static bool reading = false;    // Reading flag

static const char data_path[] = "/data";
static const char fft_path[] = "/fft";
static const char record_count_file[] = "record_count";
static const char vibration_data_file[] = "vibration_data";

lfs_t lfs;             // littlefs instance
lfs_file_t file;       // littlefs file instance
lfs_dir_t dir;         // littlefs folder instance
struct lfs_config cfg; // littlefs configuration instance

uint8_t lfs_read_buf[FS_READ_SIZE];      // Static buffer for reading
uint8_t lfs_prog_buf[FS_PROG_SIZE];      // Static buffer for writing
uint8_t lfs_lookahead_buf[FS_LOOK_SIZE]; // Static buffer for the lookahead

// Function to init mx25r device.
static bool app_flash_init(void);

// Function to erase a specific section of the mx25r.
// This should be used only with fft data for now.
static bool app_flash_erase(const uint32_t address, const uint32_t size);

// Function to prepare the general structure of the file system.
static bool prepare_structure(void);

// Function to get the size of a folder in bytes.
static uint32_t get_folder_size(const char * path);

// Function to clean up data files.
static bool clean_files(const char * path, uint16_t file_count);

// Function to remove all files from a folder.
static bool remove_files(const char * path);

// Functions needed by littlefs for reading, writing, erasisng and syncing the NOR Flash.
static int block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
static int block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
static int block_device_erase(const struct lfs_config *c, lfs_block_t block);
static int block_device_sync(const struct lfs_config *c);

// Configuration in UICR
void app_flash_import_config(void)
{
  // Getting configuration from UICR. 
  struct app_config_t config = {
    .lora_heartbeat_period_minutes = (uint8_t) app_uicr_get(UICR_LORA_HB_PERIOD_MINUTES_ID),
    .record_duration_seconds       = (uint16_t) app_uicr_get(UICR_RECORD_DURATION_SECONDS_ID),
    .record_period_minutes         = (uint16_t) app_uicr_get(UICR_RECORD_PERIOD_MINUTES_ID),
    .fft_period_hours              = (uint8_t) app_uicr_get(UICR_FFT_PERIOD_HOURS_ID),
    .accelerometer_range           = (uint8_t) app_uicr_get(UICR_ACCELEROMETER_RANGE_ID),
    .gyroscope_range               = (uint8_t) app_uicr_get(UICR_GYROSCOPE_RANGE_ID),
    .imu_frequency                 = (uint8_t) app_uicr_get(UICR_IMU_FREQUENCY_ID),
    .ch0_drive_current             = (uint8_t) app_uicr_get(UICR_CH0_DRIVE_CURRENT_ID),
    .ch1_drive_current             = (uint8_t) app_uicr_get(UICR_CH1_DRIVE_CURRENT_ID),
    .ch0_settle_count              = (uint16_t) app_uicr_get(UICR_CH0_SETTLE_COUNT_ID),
    .ch1_settle_count              = (uint16_t) app_uicr_get(UICR_CH1_SETTLE_COUNT_ID),
    .ch_enabled_bitmask            = (uint8_t) app_uicr_get(UICR_CH_ENABLED_BITMASK_ID),
  };

  app_settings_set_configuration((uint8_t *)&config);
}

void app_flash_set_default_config(void)
{
  // Setting default configuration in UICR.
  app_uicr_set(UICR_LORA_HB_PERIOD_MINUTES_ID , (uint32_t) LORA_HB_PERIOD_MINUTES_DEFAULT);
  app_uicr_set(UICR_RECORD_DURATION_SECONDS_ID, (uint32_t) RECORD_DURATION_SECONDS_DEFAULT);
  app_uicr_set(UICR_RECORD_PERIOD_MINUTES_ID  , (uint32_t) RECORD_PERIOD_MINUTES_DEFAULT);
  app_uicr_set(UICR_FFT_PERIOD_HOURS_ID       , (uint32_t) FFT_PERIOD_HOURS_DEFAULT);
  app_uicr_set(UICR_ACCELEROMETER_RANGE_ID    , (uint32_t) ACCELEROMETER_RANGE_DEFAULT);
  app_uicr_set(UICR_GYROSCOPE_RANGE_ID        , (uint32_t) GYROSCOPE_RANGE_DEFAULT);
  app_uicr_set(UICR_IMU_FREQUENCY_ID          , (uint32_t) IMU_FREQUENCY_DEFAULT);
  app_uicr_set(UICR_CH0_DRIVE_CURRENT_ID      , (uint32_t) CH0_DRIVE_CURRENT_DEFAULT);
  app_uicr_set(UICR_CH1_DRIVE_CURRENT_ID      , (uint32_t) CH1_DRIVE_CURRENT_DEFAULT);
  app_uicr_set(UICR_CH0_SETTLE_COUNT_ID       , (uint32_t) CH0_SETTLE_COUNT_DEFAULT);
  app_uicr_set(UICR_CH1_SETTLE_COUNT_ID       , (uint32_t) CH1_SETTLE_COUNT_DEFAULT);
  app_uicr_set(UICR_CH_ENABLED_BITMASK_ID     , (uint32_t) CH_ENABLED_BITMASK_DEFAULT);
}

bool app_flash_save_config(void)
{
  // Saving current configuration in UICR.
  struct app_config_t dev_cfg = {0};
  app_settings_get_configuration((uint8_t *)&dev_cfg);
  app_uicr_set(UICR_LORA_HB_PERIOD_MINUTES_ID , (uint32_t) dev_cfg.lora_heartbeat_period_minutes);
  app_uicr_set(UICR_RECORD_DURATION_SECONDS_ID, (uint32_t) dev_cfg.record_duration_seconds);
  app_uicr_set(UICR_RECORD_PERIOD_MINUTES_ID  , (uint32_t) dev_cfg.record_period_minutes);
  app_uicr_set(UICR_FFT_PERIOD_HOURS_ID       , (uint32_t) dev_cfg.fft_period_hours);
  app_uicr_set(UICR_ACCELEROMETER_RANGE_ID    , (uint32_t) dev_cfg.accelerometer_range);
  app_uicr_set(UICR_GYROSCOPE_RANGE_ID        , (uint32_t) dev_cfg.gyroscope_range);
  app_uicr_set(UICR_IMU_FREQUENCY_ID          , (uint32_t) dev_cfg.imu_frequency);
  app_uicr_set(UICR_CH0_DRIVE_CURRENT_ID      , (uint32_t) dev_cfg.ch0_drive_current);
  app_uicr_set(UICR_CH1_DRIVE_CURRENT_ID      , (uint32_t) dev_cfg.ch1_drive_current);
  app_uicr_set(UICR_CH0_SETTLE_COUNT_ID       , (uint32_t) dev_cfg.ch0_settle_count);
  app_uicr_set(UICR_CH1_SETTLE_COUNT_ID       , (uint32_t) dev_cfg.ch1_settle_count);
  app_uicr_set(UICR_CH_ENABLED_BITMASK_ID     , (uint32_t) dev_cfg.ch_enabled_bitmask);
  return true;
}

bool app_flash_enable(void)
{
  int err = 0;

  // Init SPI
  if (app_spi_init() != NRF_SUCCESS)
  {
    #if APP_FLASH_VERBOSE >= 1
    NRF_LOG_ERROR("SPI init failed.");
    #endif
    return false;
  }

  // Init mx25r device
  if (!app_flash_init()){
    #if APP_FLASH_VERBOSE >= 1
    NRF_LOG_ERROR("Flash init failed.");
    #endif
    return false;
  }
  
  // Block device operations
  cfg.read = block_device_read;   // Function to read
  cfg.prog = block_device_prog;   // Function to write
  cfg.erase = block_device_erase; // Function to erase
  cfg.sync = block_device_sync;   // Function to sync (Return 0 for now)

  // Block device configurations
  cfg.read_size = FS_READ_SIZE;      // Minimum size of a block read in bytes.
  cfg.prog_size = FS_PROG_SIZE;      // Minimum size of a block program in bytes.
  cfg.block_size = FS_BLOCK_SIZE;    // Sector size
  cfg.block_count = FS_BLOCK_COUNT;  // Sector count (24 others for FFT data)
  cfg.cache_size = FS_CACHE_SIZE;    // Size of block caches in bytes. Each cache buffers a portion of a block in
                                     // RAM. The littlefs needs a read cache, a program cache, and one additional
                                     // cache per file. Larger caches can improve performance by storing more
                                     // data and reducing the number of disk accesses. Must be a multiple of the
                                     // read and program sizes, and a factor of the block size.
  cfg.lookahead_size = FS_LOOK_SIZE; // Size of the lookahead buffer in bytes. A larger lookahead buffer
                                     // increases the number of blocks found during an allocation pass. The
                                     // lookahead buffer is stored as a compact bitmap, so each byte of RAM
                                     // can track 8 blocks. Must be a multiple of 8.
  cfg.block_cycles = FS_BLOCK_CYCLE; // Number of erase cycles before littlefs evicts metadata logs and moves
                                     // the metadata to another block. Suggested values are in the
                                     // range 100-1000, with large values having better performance at the cost
                                     // of less consistent wear distribution.
  
  // Setting static buffers. No dynamic allocation will be performed in this case.
  cfg.read_buffer = lfs_read_buf;
  cfg.prog_buffer = lfs_prog_buf;
  cfg.lookahead_buffer = lfs_lookahead_buf;
  
  // Mounting filesystem.
  err = lfs_mount(&lfs, &cfg);

  // Reformat if we can't mount the filesystem this should only happen on the first boot 
  // or after a complete erase of the NOR Flash.
  if (err < 0){
    // Erasing whole NOR flash before formatting.
    mx25r_flash_clear_all();      // Erase the NOR flash.

    NRF_LOG_ERROR("Mounting filesystem failed. Format needed.");
    err = lfs_format(&lfs, &cfg);
    if (err < 0){
      NRF_LOG_ERROR("Formatting error.");
      return false;
    }
    err = lfs_mount(&lfs, &cfg);
    if (err < 0){
      NRF_LOG_ERROR("Mounting after formatting error.");
      return false;
    }
    // Prepare the folder and file structure. 
    // This step is needed after reformating the NOR flash.
    if (!prepare_structure()){
      NRF_LOG_ERROR("Error while preparing the file structure.");
      return false;
    }
  }

  return true;
}

void app_flash_disable(void)
{
  // Unmounting
  lfs_unmount(&lfs);
  // Uninit SPI interface
  app_spi_uninit();
}

// TEST
bool app_flash_test(void)
{
  // Flash test.
  int err = 0;
  char test_filename[] = "test";
  char test_string[] = "Lynkz Instruments";
  uint8_t reading_buffer[sizeof(test_string)] = {0};
  
  // Writing the test data to the file.
  err = lfs_file_open(&lfs, &file, test_filename, LFS_O_WRONLY | LFS_O_CREAT); // Open/Create test file in write only.
  if(err < 0){ return false; }
  err = lfs_file_rewind(&lfs, &file); // Seek to the start of the file.
  if(err < 0){ return false; }
  err = lfs_file_write(&lfs, &file, test_string, sizeof(test_string)); // Reset to 0.
  if(err < 0){ return false; }
  err = lfs_file_close(&lfs, &file); // Close file.
  if(err < 0){ return false; }
  
  // Reading the data from file.
  err = lfs_file_open(&lfs, &file, test_filename, LFS_O_RDONLY); // Open the test file in read only
  if(err < 0){ return false; }
  err = lfs_file_rewind(&lfs, &file); // Seek to the start of the file.
  if(err < 0){ return false; }
  err = lfs_file_read(&lfs, &file, reading_buffer, sizeof(reading_buffer));
  if(err < 0){ return false; }
  err = lfs_file_close(&lfs, &file); // Close file
  if(err < 0){ return false; }

  // Removing the test file.
  err = lfs_remove(&lfs, test_filename);
  if(err < 0){ return false; }

  // Compare test data and reading data.
  for (uint8_t i = 0; i < sizeof(test_string); ++i){
    if( test_string[i] != reading_buffer[i]) 
    {
      return false;
    }
  }

  return true;
}

// DATA
bool app_flash_create_data_session(uint16_t * session_count)
{
  int err = 0;

  // Invalid input pointer.
  if (session_count == NULL){ return false; }
  
  // Do not create a session while recording.
  if(is_app_flash_recording()){ return false; }

  // Getting record count from file.
  struct app_record_count_t record_count = {
    .session_count = 0,
    .fft_count = 0
  };
  err = lfs_file_open(&lfs, &file, record_count_file, LFS_O_RDWR); // Open record_count file.
  if(err < 0){ return false; }
  err = lfs_file_read(&lfs, &file, &record_count, sizeof(record_count)); // Read record count.
  if(err < 0){ return false; }

  // Validate the session count. If it's 65535, the session won't be recorded.
  // The average will be sent anyway.
  if (record_count.session_count == 0xFFFF){
    NRF_LOG_INFO("Record count max, no more record.");
    *session_count = record_count.session_count;
    return false;
  }
  // Increase session count by one.
  record_count.session_count += 1; 

  err = lfs_file_rewind(&lfs, &file); // Seek to the start of the file.
  if(err < 0){ return false; }
  err = lfs_file_write(&lfs, &file, &record_count, sizeof(record_count)); // Update record_count file.
  if(err < 0){ return false; }
  err = lfs_file_close(&lfs, &file); // Close file.
  if(err < 0){ return false; }

  // Getting session id for this record.
  *session_count = record_count.session_count - 1;

  // Verify if space is still available for the data.
  // Before creating a new session, make sure there is not to much sessions.
  // If so, remove the file with the smallest id greater than 1 to keep the two first files.
  // The next data will now have space to be written.
  while(get_folder_size(data_path) + (app_settings_get_record_duration_seconds() * sizeof(struct app_packet_t)) > MAX_DATA_FOLDER_SIZE){
    NRF_LOG_INFO("Data space full, clean up needed.");
    if (!clean_files(data_path, record_count.session_count / FILE_RECORD_COUNT)){
      return false;
    }
  }

  // Prepare filename according to the number of sessions.
  // Each file can contains a total of n sessions. If this count is reached, 
  // a new file will be created with a new index. 
  uint8_t filename = (record_count.session_count - 1) / FILE_RECORD_COUNT;
  char filename_str[MAX_FILE_NAME];
  sprintf(filename_str, "%s/%d", data_path, filename);
  if ((record_count.session_count - 1) % FILE_RECORD_COUNT == 0){
     NRF_LOG_INFO("Creating file: %s", filename_str);
  }
  // Open or create data file (Create file if doesn't exist, write only and appending at each write operation).
  err = lfs_file_open(&lfs, &file, filename_str, LFS_O_WRONLY | LFS_O_APPEND | LFS_O_CREAT);
  if(err < 0){ return false; }

  // Set recording flag.
  app_flash_set_recording(true);

  return true;
}

bool app_flash_record_data_packet(uint8_t * data, uint32_t size)
{
  int err = 0;
  if (data == NULL){ return false; }
  if(is_app_flash_recording()){
    // Writing data to the file.
    err =  lfs_file_write(&lfs, &file, data, size);
    if (err < 0){
      NRF_LOG_ERROR("Error when writing: %d", err);
      return false;
    }
    return true;
  }
  return false;
}

bool app_flash_close_data_session(void)
{
  int err = 0;
  if(is_app_flash_recording()){
    // Close file.
    err = lfs_file_close(&lfs, &file);
    app_flash_set_recording(false);
    if(err < 0){ return false; }
  }

  // For now, it's always returning true.
  return true;
}

uint16_t app_flash_get_data_session_count(void)
{
  int err = 0;

  // Getting record count.
  struct app_record_count_t record_count = {
    .session_count = 0,
    .fft_count = 0
  };

  err = lfs_file_open(&lfs, &file, record_count_file, LFS_O_RDONLY); // Open record_count file as read only.
  if(err < 0){ return 0; }
  err = lfs_file_read(&lfs, &file, &record_count, sizeof(record_count)); // Read record count.
  if(err < 0){ return 0; }
  err = lfs_file_close(&lfs, &file); // Close the file.
  if(err < 0){ return 0; }
  return record_count.session_count;
}

bool app_flash_download_data_file_start(uint8_t file_id, uint32_t * data_count)
{
  if (data_count == NULL){ return false; }
  
  int err = 0;
  char filename_str[MAX_FILE_NAME];
  sprintf(filename_str, "%s/%d", data_path, file_id);
  NRF_LOG_INFO("Downloading %s", filename_str);
  
  err = lfs_file_open(&lfs, &file, filename_str, LFS_O_RDONLY); // Opening the file to download in read only.
  if (err < 0){ return false; }

  // Getting file data count
  uint32_t file_size = lfs_file_size(&lfs, &file);
  if (file_size < 0){ return false; }
  *data_count = (uint32_t) (file_size / sizeof(struct app_packet_t));
  NRF_LOG_INFO("Data count: %d", *data_count);
  
  // Set reading flag.
  app_flash_set_reading(true);

  return true;
}

bool app_flash_download_data(uint32_t index, uint8_t * data, uint32_t size)
{
  int err = 0;
  if (data == NULL){ return false; }
  if (is_app_flash_reading()){
    err = lfs_file_seek(&lfs, &file, index * size, LFS_SEEK_SET); // Seek the file at appropriate index.
    if (err < 0){ return false; }
    err = lfs_file_read(&lfs, &file, data, size);
    if (err < 0){
      NRF_LOG_INFO("Error when reading: %d", err);
      return false;
    }
    return true;
  }
  return false;
}

bool app_flash_download_data_file_stop(void)
{
  int err = 0;
  if (is_app_flash_reading()){
    // Close file.
    err = lfs_file_close(&lfs, &file);
    app_flash_set_reading(false);
    if (err < 0){ return false; }
  }
  return true;
}

bool app_flash_remove_data_sessions(void)
{
  int err = 0;

  if(!is_app_flash_erasing() && !is_app_flash_reading() && !is_app_flash_recording()){
    
    // Set erasing flag.
    app_flash_set_erasing(true);
    
    if (!remove_files(data_path)){ return false; }

    struct app_record_count_t record_count = {
      .session_count = 0,
      .fft_count = 0
    };
    err = lfs_file_open(&lfs, &file, record_count_file, LFS_O_RDWR); // Open record_count file as read and write.
    if(err < 0){ return false; }
    err = lfs_file_read(&lfs, &file, &record_count, sizeof(record_count)); // Read record count.
    if(err < 0){ return false; }
    record_count.session_count = 0; // Reset to 0.
    err = lfs_file_rewind(&lfs, &file);
    if(err < 0){ return false; }
    err = lfs_file_write(&lfs, &file, &record_count, sizeof(record_count));
    if(err < 0){ return false; }
    err = lfs_file_close(&lfs, &file); // Close file.
    if(err < 0){ return false; }
    
    // Unset erasing flag.
    app_flash_set_erasing(false);
    
    return true;
  }
  
  #if APP_FLASH_VERBOSE >= 1
  NRF_LOG_ERROR("Flash erase data FAILED, flash busy");
  #endif

  return false;
}

// FFT
bool app_flash_create_fft_session(uint16_t * session_count, struct app_fft_header_t header)
{
  int err = 0;

  // Invalid input pointer.
  if (session_count == NULL){ return false; }
  
  // Do not create a session while recording.
  if(is_app_flash_recording()){ return false; }

  // Getting record count from file.
  struct app_record_count_t record_count = {
    .session_count = 0,
    .fft_count = 0
  };
  err = lfs_file_open(&lfs, &file, record_count_file, LFS_O_RDWR); // Open record_count file.
  if(err < 0){ return false; }
  err = lfs_file_read(&lfs, &file, &record_count, sizeof(record_count)); // Read record count.
  if(err < 0){ return false; }

  // Validate the session count. If it's 65535, the session won't be recorded.
  // The average will be sent anyway.
  if (record_count.fft_count == 0xFFFF){
    NRF_LOG_INFO("Record count max, no more record.");
    *session_count = record_count.fft_count;
    return false;
  }
  // Increase fft session count by one.
  record_count.fft_count += 1; 

  err = lfs_file_rewind(&lfs, &file); // Seek to the start of the file.
  if(err < 0){ return false; }
  err = lfs_file_write(&lfs, &file, &record_count, sizeof(record_count)); // Update record_count file.
  if(err < 0){ return false; }
  err = lfs_file_close(&lfs, &file); // Close file.
  if(err < 0){ return false; }

  // Getting session id for this record.
  *session_count = record_count.fft_count - 1;

  // Verify if space is still available for the fft.
  // Before creating a new session, make sure there is not to much sessions.
  // If so, remove the file with the smallest id greater than 1 to keep the two first files.
  // The next data will now have space to be written.
  header.fft_id = *session_count;
  while(get_folder_size(fft_path) + (FFT_SIZE) + sizeof(header) > MAX_DATA_FOLDER_SIZE){
    NRF_LOG_INFO("Data space full, clean up needed.");
    if (!clean_files(fft_path, record_count.fft_count / FFT_FILE_RECORD_COUNT)){
      return false;
    }
  }

  // Prepare filename according to the number of sessions.
  // Each file can contains a total of n sessions. If this count is reached, 
  // a new file will be created with a new index. 
  uint8_t filename = (record_count.fft_count - 1) / FFT_FILE_RECORD_COUNT;
  char filename_str[MAX_FILE_NAME];
  sprintf(filename_str, "%s/%d", fft_path, filename);
  if ((record_count.fft_count - 1) % FILE_RECORD_COUNT == 0){
     NRF_LOG_INFO("Creating file: %s", filename_str);
  }
  // Open or create data file (Create file if doesn't exist, write only and appending at each write operation).
  err = lfs_file_open(&lfs, &file, filename_str, LFS_O_WRONLY | LFS_O_APPEND | LFS_O_CREAT);
  if(err < 0){ return false; }

  // Write the FFT header to the file.
  err = lfs_file_write(&lfs, &file, (uint8_t *)&header, sizeof(header));
  if(err < 0){ return false; }

  // Set recording flag.
  app_flash_set_recording(true);

  return true;
}

bool app_flash_record_fft_packet(uint8_t * data, uint32_t size)
{
  int err = 0;
  if (data == NULL){ return false; }
  if(is_app_flash_recording()){
    // Writing data to the file.
    err =  lfs_file_write(&lfs, &file, data, size);
    if (err < 0){
      NRF_LOG_ERROR("Error when writing: %d", err);
      return false;
    }
    return true;
  }
  return false;
}

bool app_flash_close_fft_session(void)
{
  int err = 0;
  if(is_app_flash_recording()){
    // Close file.
    err = lfs_file_close(&lfs, &file);
    app_flash_set_recording(false);
    if(err < 0){ return false; }
  }

  // For now, it's always returning true.
  return true;
}

uint16_t app_flash_get_fft_session_count(void)
{
  int err = 0;

  // Getting record count.
  struct app_record_count_t record_count = {
    .session_count = 0,
    .fft_count = 0
  };

  err = lfs_file_open(&lfs, &file, record_count_file, LFS_O_RDONLY); // Open record_count file as read only.
  if(err < 0){ return 0; }
  err = lfs_file_read(&lfs, &file, &record_count, sizeof(record_count)); // Read record count.
  if(err < 0){ return 0; }
  err = lfs_file_close(&lfs, &file); // Close the file.
  if(err < 0){ return 0; }
  return record_count.fft_count;
}

bool app_flash_download_fft_file_start(uint8_t file_id, uint32_t * data_count)
{
  if (data_count == NULL){ return false; }
  
  int err = 0;
  char filename_str[MAX_FILE_NAME];
  sprintf(filename_str, "%s/%d", fft_path, file_id);
  NRF_LOG_INFO("Downloading %s", filename_str);
  
  err = lfs_file_open(&lfs, &file, filename_str, LFS_O_RDONLY); // Opening the file to download in read only.
  if (err < 0){ return false; }

  // Getting file data count
  uint32_t file_size = lfs_file_size(&lfs, &file);
  if (file_size < 0){ return false; }
  *data_count = (uint32_t) (file_size / (sizeof(struct app_fft_header_t) + FFT_SIZE));
  NRF_LOG_INFO("Data count: %d", *data_count);
  
  // Set reading flag.
  app_flash_set_reading(true);

  return true;
}

bool app_flash_download_fft(uint32_t index, uint8_t * data, uint32_t size)
{
  int err = 0;
  if (data == NULL){ return false; }
  if (is_app_flash_reading()){
    err = lfs_file_seek(&lfs, &file, index, LFS_SEEK_SET); // Seek the file at appropriate index.
    if (err < 0){ return false; }
    err = lfs_file_read(&lfs, &file, data, size);
    if (err < 0){
      NRF_LOG_INFO("Error when reading: %d", err);
      return false;
    }
    return true;
  }
  return false;
}

bool app_flash_download_fft_file_stop(void)
{
  int err = 0;
  if (is_app_flash_reading()){
    // Close file.
    err = lfs_file_close(&lfs, &file);
    app_flash_set_reading(false);
    if (err < 0){ return false; }
  }
  return true;
}

bool app_flash_remove_fft_sessions(void)
{
  int err = 0;

  if(!is_app_flash_erasing() && !is_app_flash_reading() && !is_app_flash_recording()){
    
    // Set erasing flag.
    app_flash_set_erasing(true);
    
    if (!remove_files(fft_path)){ return false; }

    struct app_record_count_t record_count = {
      .session_count = 0,
      .fft_count = 0
    };
    err = lfs_file_open(&lfs, &file, record_count_file, LFS_O_RDWR); // Open record_count file as read and write.
    if(err < 0){ return false; }
    err = lfs_file_read(&lfs, &file, &record_count, sizeof(record_count)); // Read record count.
    if(err < 0){ return false; }
    record_count.fft_count = 0; // Reset to 0.
    err = lfs_file_rewind(&lfs, &file);
    if(err < 0){ return false; }
    err = lfs_file_write(&lfs, &file, &record_count, sizeof(record_count));
    if(err < 0){ return false; }
    err = lfs_file_close(&lfs, &file); // Close file.
    if(err < 0){ return false; }
    
    // Unset erasing flag.
    app_flash_set_erasing(false);
    
    return true;
  }
  
  #if APP_FLASH_VERBOSE >= 1
  NRF_LOG_ERROR("Flash erase data FAILED, flash busy");
  #endif

  return false;
}

bool app_flash_remove_fft_data(void)
{
  // Erase last FFT content.
  uint32_t bytes_to_erase = FFT_TOTAL_RECORD;
  if( bytes_to_erase % 0x1000)
  {
    bytes_to_erase = ((bytes_to_erase / 0x1000) + 1) * 0x1000;
  }
  return app_flash_erase(FFT_ANALYZER_ADDR, bytes_to_erase); 
}

bool app_flash_append_vibration_data(uint16_t data)
{
  int err = 0;
  err = lfs_file_open(&lfs, &file, vibration_data_file, LFS_O_WRONLY | LFS_O_APPEND | LFS_O_CREAT);
  if(err < 0){ return false; }
  err = lfs_file_write(&lfs, &file, (uint8_t *)&data, sizeof(data));
  if(err < 0){ return false; }
  err = lfs_file_close(&lfs, &file);
  if(err < 0){ return false; }
  return true;
}

uint32_t app_flash_get_vibration_data_size(void)
{
  int err = 0;
  uint32_t file_size = 0;
  err = lfs_file_open(&lfs, &file, vibration_data_file, LFS_O_WRONLY | LFS_O_APPEND | LFS_O_CREAT);
  if(err < 0){ return 0; }
  file_size = lfs_file_size(&lfs, &file);
  if(file_size < 0){ return 0; }
  err = lfs_file_close(&lfs, &file);
  if(err < 0){ return 0; }
  return file_size;
}

bool app_flash_get_vibration_data(uint8_t * data, uint8_t size)
{
  int err = 0;
  err = lfs_file_open(&lfs, &file, vibration_data_file, LFS_O_RDONLY);
  if(err < 0){ return false; }
  err = lfs_file_rewind(&lfs, &file); // Seek to the start of the file.
  if(err < 0){ return false; }
  err = lfs_file_read(&lfs, &file, data, size);
  if(err < 0){ return false; }
  err = lfs_file_close(&lfs, &file);
  if(err < 0){ return false; }
  return true;
}

bool app_flash_remove_vibration_data(void)
{
  int err = 0;
  err = lfs_remove(&lfs, vibration_data_file);
  if (err < 0){ return false; }
  return true;
}

bool app_flash_erase_all(void)
{
  if(!is_app_flash_erasing() && !is_app_flash_reading() && !is_app_flash_recording()){

    #if APP_FLASH_VERBOSE >= 2
    NRF_LOG_INFO("Flash erase all");
    #endif

    app_flash_set_erasing(true);  // Set erasing flag.
    mx25r_flash_clear_all();      // Erase the NOR flash.
    app_flash_set_erasing(false); // Unset erasing flag.
    return true;
  }

  #if APP_FLASH_VERBOSE >= 1
  NRF_LOG_ERROR("Flash erase all FAILED, flash busy");
  #endif

  return false;
}

uint8_t app_flash_get_percentage(void)
{
  int err = 0;
  err = lfs_fs_size(&lfs);
  if (err < 0){
    return 0;
  }
  float block_count = (float)FS_BLOCK_COUNT;
  float percentage = (float)err / block_count;
  NRF_LOG_INFO("%d/%d", err, FS_BLOCK_COUNT);
  uint8_t percent = (uint8_t)(percentage * 100.0f);
  return percent;
}

void app_flash_set_erasing(bool value)
{
  erasing = value;
}

void app_flash_set_recording(bool value)
{
  recording = value;
}

void app_flash_set_reading(bool value)
{
  reading = value;
}

bool is_app_flash_erasing(void)
{
  return erasing;
}

bool is_app_flash_recording(void)
{
  return recording;
}

bool is_app_flash_reading(void)
{
  return reading;
}

static bool app_flash_init(void)
{
  // Initialize the mx25r device.
  if(mx25r_dev_init(&app_spi_instance, SPIM1_CSB_FLASH_PIN, 0x20, 0x16) == MX25R_OK){
    return true;
  }
  return false;
}

static bool app_flash_erase(const uint32_t address, const uint32_t size)
{
  if( address % 0x1000) return false;
  if( size % 0x1000) return false;

  for(uint32_t i = 0 ; i < size; i+=0x1000){
    mx25r_flash_clear_sector(address + i);
  }

  return true;
}

static bool prepare_structure(void)
{
  int err = 0;

  // Creating folders /data/ and /fft/
  err = lfs_mkdir(&lfs, data_path);
  if (err < 0){ return false; }
  err = lfs_mkdir(&lfs, fft_path);
  if (err < 0){ return false; }

  // Creating and resetting record_count file.
  struct app_record_count_t record_count = {
    .session_count = 0,
    .fft_count = 0
  };

  err = lfs_file_open(&lfs, &file, record_count_file, LFS_O_WRONLY | LFS_O_CREAT); // Open/Create record_count file.
  if (err < 0){ return false; }
  err = lfs_file_rewind(&lfs, &file); 
  if (err < 0){ return false; }
  err = lfs_file_write(&lfs, &file, &record_count, sizeof(record_count)); // Reset to 0.
  if (err < 0){ return false; }
  err = lfs_file_close(&lfs, &file); // Close file.
  if (err < 0){ return false; }
  
  return true;
}

static uint32_t get_folder_size(const char * path)
{
  //NRF_LOG_INFO("Session folder informations:");
  int err = 0;
  uint32_t total_size = 0;
  err = lfs_dir_open(&lfs, &dir, path);
  struct lfs_info info;
  while(true){
    err = lfs_dir_read(&lfs, &dir, &info);  
    if (err < 0){
      NRF_LOG_INFO("Error reading directory %s: %d", path, err);
    }
    if (err == 0){
      break;
    }
    
    if (info.type == LFS_TYPE_REG){
      // NRF_LOG_INFO("Size of %s/%s: %d", path, info.name, info.size);
      total_size += info.size;
    }
  }
  lfs_dir_close(&lfs, &dir);
  NRF_LOG_INFO("Total size of %s: %d", path, total_size);
  return total_size;
}

static bool clean_files(const char * path, uint16_t file_count)
{
  NRF_LOG_INFO("Cleaning data files...");
  int err = 0;
  
  // Getting the smallest file id greater than 1.
  uint16_t file_index = 0;
  uint16_t smaller_index = file_count;

  err = lfs_dir_open(&lfs, &dir, path);
  struct lfs_info info;
  while(true){
    err = lfs_dir_read(&lfs, &dir, &info);  
    if (err < 0){
      NRF_LOG_INFO("Error reading directory %s: %d", path, err);
    }
    if (err == 0){
      break;
    }
    
    if (info.type == LFS_TYPE_REG){
      file_index = (uint16_t)atoi(info.name);
      NRF_LOG_INFO("File id: %d", file_index);
      if (smaller_index > file_index && file_index > 1){
        smaller_index = file_index;
      }
    }
  }
  lfs_dir_close(&lfs, &dir);

  // There is only files 0 and 1, cannot remove them. 
  // The next data won't be in files to make sure to keep these two files.
  if (smaller_index == 1 || smaller_index == 0){
    return false;
  }

  // Remove the file with this index to make space.
  char filename_str[MAX_FILE_NAME];
  sprintf(filename_str, "%s/%d", path, smaller_index);
  NRF_LOG_INFO("Deleting %s", filename_str);
  err = lfs_remove(&lfs, filename_str);

  return true;
}

static bool remove_files(const char * path)
{
  int err = 0;
  // Opening data directory.
  struct lfs_info info;
  err = lfs_dir_open(&lfs, &dir, path);
  if(err < 0){ return false; }
  
  // Passing through all the directory elements.
  while(true){
    err = lfs_dir_read(&lfs, &dir, &info);  
    if (err < 0){
      NRF_LOG_INFO("Error reading directory %s: %d", path, err);
      return false;
    }
    if (err == 0){ // End of the elements.
      break;
    }
    // If the element is a file. 
    if (info.type == LFS_TYPE_REG){
      char filename_str[MAX_FILE_NAME];
      sprintf(filename_str, "%s/%s", path, info.name);
      NRF_LOG_INFO("Deleting %s", filename_str);
      err = lfs_remove(&lfs, filename_str);
      if(err < 0){ return false; }
    }
  }
  err = lfs_dir_close(&lfs, &dir); // Closing directory.
  if(err < 0){ return false; }
  return true;
}

static int block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
  // mx25r function to read.
  mx25r_flash_read(((block + FS_BLOCK_OFFSET) * c->block_size + off), (uint8_t *)buffer, size);
  return 0;
}

static int block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
  // mx25r function to write.
  mx25r_flash_write((uint8_t *)buffer, ((block + FS_BLOCK_OFFSET) * c->block_size + off), size);
  return 0;
}

static int block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
  // mx25r function to clear a sector.
  mx25r_flash_clear_sector((block + FS_BLOCK_OFFSET) * c->block_size);
  return 0;
}

static int block_device_sync(const struct lfs_config *c)
{
  // As per the littlefs documentation: If your storage caches writes, make sure that the provided sync function 
  // flushes all the data to memory and ensures that the next read fetches the data from memory, otherwise data 
  // integrity can not be guaranteed. If the write function does not perform caching, and therefore each read or 
  // write call hits the memory, the sync function can simply return 0.
  return 0;
}
