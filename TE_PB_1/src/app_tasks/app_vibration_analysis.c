/**
 * @file app_vibration_analysis.c
 * @author Alexandre Desgagné (alexd@lynkz.ca)
 * @brief
 * @version 1.0
 * @date 2024-02-06
 *
 * @copyright Copyright (c) Lynkz Instruments Inc. Amos 2023
 */

#include "app_vibration_analysis.h"

#include "math.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "arm_const_structs.h"

#include "app.h"
#include "app_imu.h"
#include "app_flash.h"
#include "app_hardware.h"
#include "app_flash.h"
#include "app_settings.h"
#include "hamming.h"
#include "mx25r_nrf5.h"

#define MAX_INT16_T 32767

int16_t fft_gain = 1;
uint8_t fft_freq = APP_IMU_FREQ_1600HZ;

void remove_dc_from_signal(int16_t *signal, uint32_t size)
{
  int total = 0;

  for (int  i = 0; i < size/2; i++){
    total =  signal[i*2] + total;
  }

  int mean = total / (int)(size/2);

  for (int  i = 0; i < size/2; i++){
    signal[i*2] =  signal[i*2] - mean;
  }
}

void hamming_windowing(int16_t *signal, uint32_t size)
{
  int i = 0;
  for (i = 0 ; i < size/2; i++){
    signal[i*2] = (float)signal[i*2] * hamming[i];
  }
}

void do_fft_4096_samples(int16_t *signal)
{
  // Do FFT
  arm_cfft_q15(&arm_cfft_sR_q15_len4096, signal, 0, 1);

  // Do absolute of FFT : pythagorian of real and imaginary numbers.
  for (int i = 0; i < FFT_SIZE/2; i++){
    signal[i] = sqrt(pow(signal[i*2],2) + pow(signal[i*2+1],2));
  }

  // Only keep positive spectrum of the fft.
  memset(&signal[FFT_SIZE*2/4], 0, FFT_SIZE * 2 * 3/4 * 2);  
}

void optimize_sample_range(int16_t * sample, int size, int16_t gain_fft)
{
  for (int i = 0; i < size; i++){
    sample[i] = sample[i] * gain_fft; 
  }
}

void app_vibration_fft(uint16_t * buf_out, const uint16_t buffer_size)
{
  // IMPORTANT: Flash must be enable before using this function.

  if (buf_out == NULL){ return; }
  
  // Init BMI.
  fft_freq = app_settings_get_imu_frequency(); // Get frequency used for this FFT.
  app_imu_init(true);
  
  // Gain optimisation variables
  // First reading
  struct accelerometer_sensor_data_t accel_data = {0};
  struct gyroscope_sensor_data_t gyro_data = {0};
  app_imu_read_accel_gyro(&accel_data, &gyro_data);

  int16_t accel_norm = (int16_t)sqrt(accel_data.x * accel_data.x + accel_data.y * accel_data.y + accel_data.z * accel_data.z);
  int16_t max = accel_norm;
  int16_t min = accel_norm;
  int16_t mean = 0;
  int64_t total = 0;
  int16_t mean_counter = 1;
  uint8_t buffer[FFT_FLASH_WRITE_SIZE] = {0}; // Buffer to read/write nor flash memory
  int count = 0;
  
  // Recording accel raw data in FFT data file at configured FFT.
  NRF_LOG_INFO("FFT recording");
  
  int i = 0;
  bmi_new_data_counter = 0;
  for (int n = 0; n < FFT_TOTAL_RECORD / FFT_FLASH_WRITE_SIZE; ++n){ // 6144 loops
    for(int m = 0; m < FFT_FLASH_WRITE_SIZE / 2; ++m){ // 16 loops
      // Wait the bmi data
      while(!is_imu_new_data){} 
      
      if (bmi_new_data_counter > 1)
      {
        NRF_LOG_INFO("Data skipped-> %d: %d", i, bmi_new_data_counter);
      }

      // Getting acceleration norm.
      app_imu_read_accel_gyro(&accel_data, &gyro_data);
      accel_norm = (int16_t)sqrt(accel_data.x * accel_data.x + accel_data.y * accel_data.y + accel_data.z * accel_data.z);
      // accel_norm = (int16_t)1000.0f * sinf(2.0f * M_PI * 100.0f * (float)(i) / 800.0f);

      // Compute min, max and total.
      if (accel_norm > max){ max = accel_norm; }
      if (accel_norm < min){ min = accel_norm; }
      total += accel_norm;
      count++;

      if (count % 1000 == 0){
        NRF_LOG_INFO("%d", count);
      }

      is_imu_new_data = false;
      bmi_new_data_counter = 0;

      // Fill up buffer to be written to the FFT data file.
      buffer[2*m] = accel_norm & 0x00ff; 
      buffer[2*m+1] = (accel_norm & 0xff00) >> 8;
      i++;
    }
    // Append module data in file.
    mx25r_flash_write(buffer, FFT_ANALYZER_ADDR + (n * FFT_FLASH_WRITE_SIZE), FFT_FLASH_WRITE_SIZE);
    app_hdw_wdt_kick(); // Kick wacthdog  
  }
  
  NRF_LOG_INFO("FFT recording done");

  // Uninit BMI.
  app_imu_uninit();

  NRF_LOG_INFO("FFT computing");

  // Find gain to apply to FFT to maximise amplitude thus maximising fft resolution.
  mean = total / count; 
  if(abs(mean-max) > abs(mean-min)){
    fft_gain = MAX_INT16_T / abs(mean-max);
  }
  else{
    fft_gain = MAX_INT16_T / abs(mean-min);
  }

  // Clear FFT buffers
  static int16_t fft_buffer[FFT_SIZE * 2] = {0}; // FFT_COMPUTE_LEN*2 to take into consideration imaginary number of the fft
  memset(buf_out, 0, buffer_size);

  // Do pwelch:
  //  1) Read first 4096 samples in nor flash memory to use for FFT (8192 bytes).
  //  2) Remove gravity offset in accel samples by substraction the mean of the signal to itself.
  //  3) Optimize amplitude resolution by applying a gain to the signal which will make the signal amplitude near to maximum value possible with int16_t.
  //     This will eliminate saturated effect caused by a weak acceleration signal.
  //  4) Apply a Hamming window to remove artifact frequencies caused by a rectangular window. (Spectral leakage)
  //  5) Add this fft to mean fft.
  //  6) Read next 4096 samples with an offset of N/3 to ensure an overlap of 1/3 between windows.
  
  uint32_t in_file_index = 0;

  while (FFT_TOTAL_RECORD - in_file_index >= FFT_SIZE*2){ // While there is still enough accel samples in nor flash to add to mean
    for (int i = 0; i < FFT_SIZE; i++){
      // Read next data in file.
      mx25r_flash_read(in_file_index + (2*i), buffer, 2);
      fft_buffer[i*2]  = (buffer[1] << 8 |  buffer[0]); 
    }
    
    // Removing constant effect from signal.
    remove_dc_from_signal(fft_buffer, FFT_SIZE*2);
    
    // Optimize sample range.
    optimize_sample_range(fft_buffer, FFT_SIZE*2, fft_gain);

    // Hamming window. 
    hamming_windowing(fft_buffer, FFT_SIZE*2);

    // FFT
    do_fft_4096_samples(fft_buffer);

    // Add buffer to mean.
    for (int i = 0 ; i < FFT_SIZE/2; i++){
      // Use this way to calculate the mean because summing all ffts could resolve in overflowing buf_out.
      buf_out[i] = buf_out[i] + (fft_buffer[i] - buf_out[i]) / mean_counter; 
    } 
    mean_counter++;
    
    // Setup the next starting adress.
    in_file_index = in_file_index + FFT_SIZE * 2 / 3;
    
    // Clear the fft buffer.
    memset(fft_buffer, 0, sizeof(fft_buffer));

    app_hdw_wdt_kick(); // Kick watchdog.
  }
}

uint16_t app_vibration_analyze(void)
{
  struct accelerometer_sensor_data_t accel_data = {0};
  struct gyroscope_sensor_data_t gyro_data = {0};

  app_imu_init(false); // Initialize BMI.
  app_imu_get_accel_gyro_rms(&accel_data, &gyro_data); // Getting accelerometer and gyroscope RMS.
  app_imu_uninit(); // Unitialize BMI
  return sqrt(accel_data.x * accel_data.x + accel_data.y * accel_data.y + accel_data.z * accel_data.z); // Return RMS module.
}
