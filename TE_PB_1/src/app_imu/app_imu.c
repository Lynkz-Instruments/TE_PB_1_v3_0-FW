/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/
#include "app_imu.h"

#include "app_spi.h"
#include "app_settings.h"

// IMU Sampling Defines
#define IMU_MEASUREMENT_SAMPLING_SIZE  10 // In samples number
#define IMU_MEASUREMENT_SAMPLING_RATE  5 // In ms

// Calculation Defines
#define G_TO_MG 10000 // Conversion from g to 0.1mg units

bool is_imu_new_data = false;
uint8_t bmi_new_data_counter = 0;

/**
 * @brief Compute accel rms value from accel raw buffer
 * @return int16_t 
 */
static int16_t compute_accel_rms_value(int16_t *buffer, int32_t size);

/**
 * @brief Compute gyro rms value from gyro raw buffer
 * @return int16_t 
 */
static int16_t compute_gyro_rms_value(int16_t *buffer, int32_t size);

static void imu_interrupts_init(void);

// Config Struct for BMI270.
struct bmi270_config_t bmi270_cfg = {
  .acc_rate =  BMI2_ACC_ODR_1600HZ,
  .acc_range = BMI2_ACC_RANGE_4G,
  .gyr_rate =  BMI2_GYR_ODR_1600HZ,
  .gyr_range = BMI2_GYR_RANGE_2000
};

// Initialisation routine for the IMU
bool app_imu_init(bool fft)
{
  app_spi_init();

  // Set BMI configuration structure based on device configs.
  // Accelerometer
  switch(app_settings_get_accelerometer_range()){
    case APP_IMU_ACC_2G:
      bmi270_cfg.acc_range = BMI2_ACC_RANGE_2G;
      break;
    case APP_IMU_ACC_4G:
      bmi270_cfg.acc_range = BMI2_ACC_RANGE_4G;
      break;
    case APP_IMU_ACC_8G:
      bmi270_cfg.acc_range = BMI2_ACC_RANGE_8G;
      break;
    case APP_IMU_ACC_16G:
      bmi270_cfg.acc_range = BMI2_ACC_RANGE_16G;
      break;
    default: 
      bmi270_cfg.acc_range = BMI2_ACC_RANGE_4G;
      break;
  }

  // Gyroscope
  switch(app_settings_get_gyroscope_range()){
    case APP_IMU_GYRO_125DPS:
      bmi270_cfg.gyr_range = BMI2_GYR_RANGE_125;
      break;
    case APP_IMU_GYRO_250DPS:
      bmi270_cfg.gyr_range = BMI2_GYR_RANGE_250;
      break;
    case APP_IMU_GYRO_500DPS:
      bmi270_cfg.gyr_range = BMI2_GYR_RANGE_500;
      break;
    case APP_IMU_GYRO_1000DPS:
      bmi270_cfg.gyr_range = BMI2_GYR_RANGE_1000;
      break;
    case APP_IMU_GYRO_2000DPS:
      bmi270_cfg.gyr_range = BMI2_GYR_RANGE_2000;
      break;
    default:
      bmi270_cfg.gyr_range = BMI2_GYR_RANGE_2000;
      break;
  }
  
  if(fft){
    // Frequency selection only if init for FFT. 
    // Only frequencies from 200Hz to 1600Hz can be selected.
    switch(app_settings_get_imu_frequency()){
      //case APP_IMU_FREQ_25HZ:
      //  bmi270_cfg.acc_rate = BMI2_ACC_ODR_25HZ;
      //  bmi270_cfg.gyr_rate = BMI2_GYR_ODR_25HZ;
      //  break;
      //case APP_IMU_FREQ_50HZ:
      //  bmi270_cfg.acc_rate = BMI2_ACC_ODR_50HZ;
      //  bmi270_cfg.gyr_rate = BMI2_GYR_ODR_50HZ;
      //  break;
      //case APP_IMU_FREQ_100HZ:
      //  bmi270_cfg.acc_rate = BMI2_ACC_ODR_100HZ;
      //  bmi270_cfg.gyr_rate = BMI2_GYR_ODR_100HZ;
      //  break;
      case APP_IMU_FREQ_200HZ:
        bmi270_cfg.acc_rate = BMI2_ACC_ODR_200HZ;
        bmi270_cfg.gyr_rate = BMI2_GYR_ODR_200HZ;
        break;
      case APP_IMU_FREQ_400HZ:
        bmi270_cfg.acc_rate = BMI2_ACC_ODR_400HZ;
        bmi270_cfg.gyr_rate = BMI2_GYR_ODR_400HZ;
        break;
      case APP_IMU_FREQ_800HZ:
        bmi270_cfg.acc_rate = BMI2_ACC_ODR_800HZ;
        bmi270_cfg.gyr_rate = BMI2_GYR_ODR_800HZ;
        break;
      case APP_IMU_FREQ_1600HZ:
        bmi270_cfg.acc_rate = BMI2_ACC_ODR_1600HZ;
        bmi270_cfg.gyr_rate = BMI2_GYR_ODR_1600HZ;
        break;
      default:
        bmi270_cfg.acc_rate = BMI2_ACC_ODR_1600HZ;
        bmi270_cfg.gyr_rate = BMI2_GYR_ODR_1600HZ;
        break;
    }
  }
  else{
    bmi270_cfg.acc_rate = BMI2_ACC_ODR_1600HZ;
    bmi270_cfg.gyr_rate = BMI2_GYR_ODR_1600HZ;
  }

  // Initialize IMU Device
  if(bmi270_nrf_init(&bmi270_cfg, SPIM1_CSB_IMU_PIN, &app_spi_instance, NULL, NULL)){
    // Initialize IMU Interrupts
    imu_interrupts_init();
    nrf_drv_gpiote_in_event_enable(IMU_INT_PIN, true); 
    return true;
  }
  return false;
}

// BMI270 callback when a new bmi270 data is ready
void bmi_interrupt_callback(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  is_imu_new_data = true;
  bmi_new_data_counter++;
}

static void imu_interrupts_init(void)
{
  // Interrupt PINs configuration
  struct bmi2_int_pin_config data_int_cfg;
  data_int_cfg.pin_type = BMI2_INT_BOTH;
  data_int_cfg.int_latch = BMI2_INT_NON_LATCH;
  data_int_cfg.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE; // Output enabled
  data_int_cfg.pin_cfg[0].od = BMI2_INT_PUSH_PULL;            // OpenDrain disabled
  data_int_cfg.pin_cfg[0].lvl = BMI2_INT_ACTIVE_HIGH;		// Signal Low Active
  data_int_cfg.pin_cfg[0].input_en = BMI2_INT_INPUT_DISABLE;	// Input Disabled
  data_int_cfg.pin_cfg[1].output_en = BMI2_INT_OUTPUT_ENABLE; // Output enabled
  data_int_cfg.pin_cfg[1].od = BMI2_INT_PUSH_PULL;            // OpenDrain disabled
  data_int_cfg.pin_cfg[1].lvl = BMI2_INT_ACTIVE_HIGH;		// Signal Low Active
  data_int_cfg.pin_cfg[1].input_en = BMI2_INT_INPUT_DISABLE;	// Input Disabled

  bmi2_set_int_pin_config( &data_int_cfg, &m_bmi270_dev);

  // Configure nrf52832 gpio
  ret_code_t err_code;

  err_code = nrf_drv_gpiote_init();
  APP_ERROR_CHECK(err_code);

  nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
  in_config.pull = NRF_GPIO_PIN_NOPULL;

  err_code = nrf_drv_gpiote_in_init(IMU_INT_PIN, &in_config, bmi_interrupt_callback);
  APP_ERROR_CHECK(err_code);
  
}

// Un-Initialisation routine for the IMU
void app_imu_uninit(void)
{
  nrf_drv_gpiote_uninit();
  bmi270_soft_reset();
}

// WARNING Accelerometer offsets are not taken into account here.
static int16_t compute_accel_rms_value(int16_t *buffer, int32_t size)
{
  int16_t res_MMM[3] = {0};
  findMMM(buffer, res_MMM, size);
  
  switch(bmi270_cfg.acc_range){
    case BMI2_ACC_RANGE_2G:
      return (res_MMM[2] * G_TO_MG / BMI2_ACC_FOC_2G_REF);
    case BMI2_ACC_RANGE_4G:
      return (res_MMM[2] * G_TO_MG / BMI2_ACC_FOC_4G_REF);
    case BMI2_ACC_RANGE_8G:
      return (res_MMM[2] * G_TO_MG / BMI2_ACC_FOC_8G_REF);
    case BMI2_ACC_RANGE_16G:
      return (res_MMM[2] * G_TO_MG / BMI2_ACC_FOC_16G_REF);
    default:
      return (res_MMM[2] * G_TO_MG / BMI2_ACC_FOC_4G_REF);
  }
}

static int16_t compute_gyro_rms_value(int16_t *buffer, int32_t size)
{
  int16_t res_MMM[3] = {0};
  findMMM(buffer, res_MMM, size);
  switch(bmi270_cfg.gyr_range){
    case BMI2_GYR_RANGE_125:
      return (res_MMM[2] / BMI2_GYRO_FOC_125_DPS_REF);
    case BMI2_GYR_RANGE_250:
      return (res_MMM[2] / BMI2_GYRO_FOC_250_DPS_REF);
    case BMI2_GYR_RANGE_500:
      return (res_MMM[2] / BMI2_GYRO_FOC_500_DPS_REF);
    case BMI2_GYR_RANGE_1000:
      return (res_MMM[2] / BMI2_GYRO_FOC_1000_DPS_REF);
    case BMI2_GYR_RANGE_2000:
      return (res_MMM[2] / BMI2_GYRO_FOC_2000_DPS_REF);
    default:
      return (res_MMM[2] / BMI2_GYRO_FOC_2000_DPS_REF);
  }
}

void app_imu_get_accel_gyro_rms(struct accelerometer_sensor_data_t * accel_data, struct gyroscope_sensor_data_t * gyro_data)
{
  int16_t buf_accel_x[IMU_MEASUREMENT_SAMPLING_SIZE] = {0};
  int16_t buf_accel_y[IMU_MEASUREMENT_SAMPLING_SIZE] = {0};
  int16_t buf_accel_z[IMU_MEASUREMENT_SAMPLING_SIZE] = {0};
  int16_t buf_gyro_x[IMU_MEASUREMENT_SAMPLING_SIZE] = {0};
  int16_t buf_gyro_y[IMU_MEASUREMENT_SAMPLING_SIZE] = {0};
  int16_t buf_gyro_z[IMU_MEASUREMENT_SAMPLING_SIZE] = {0};
  uint16_t int_status = 0;
  
  bmi_new_data_counter = 0;
  for(int i=0; i < IMU_MEASUREMENT_SAMPLING_SIZE;){
    // Wait bmi new data ready.
    if (is_imu_new_data){
      //// Check if some imu data were skipped.
      //// This should not happen normally.
      //if (bmi_new_data_counter > 1){
      //  NRF_LOG_INFO("Data skipped");
      //}

      // Check the interrupt status.
      if(bmi270_get_int_status(&int_status)){
        if ((int_status & BMI2_ACC_DRDY_INT_MASK) && (int_status & BMI2_GYR_DRDY_INT_MASK)){ // Data ready.
          // Read data
          bmi270_get_sensor_data(accel_data, gyro_data);

          // Store data in buffers.
          buf_accel_x[i] = accel_data->x;
          buf_accel_y[i] = accel_data->y;
          buf_accel_z[i] = accel_data->z;
          buf_gyro_x[i] = gyro_data->x;
          buf_gyro_y[i] = gyro_data->y;
          buf_gyro_z[i] = gyro_data->z;
      
          // Clear new data flag.
          is_imu_new_data = false;
          bmi_new_data_counter = 0;
          ++i;
        }
      }
    }
    __WFE();
  }

  // Compute RMS of each buffers.
  accel_data->x = compute_accel_rms_value(buf_accel_x, IMU_MEASUREMENT_SAMPLING_SIZE);
  accel_data->y = compute_accel_rms_value(buf_accel_y, IMU_MEASUREMENT_SAMPLING_SIZE);
  accel_data->z = compute_accel_rms_value(buf_accel_z, IMU_MEASUREMENT_SAMPLING_SIZE);
  gyro_data->x = compute_gyro_rms_value(buf_gyro_x, IMU_MEASUREMENT_SAMPLING_SIZE);
  gyro_data->y = compute_gyro_rms_value(buf_gyro_y, IMU_MEASUREMENT_SAMPLING_SIZE);
  gyro_data->z = compute_gyro_rms_value(buf_gyro_z, IMU_MEASUREMENT_SAMPLING_SIZE);
}

bool app_imu_read_accel_gyro(struct accelerometer_sensor_data_t * accel_data, struct gyroscope_sensor_data_t * gyro_data)
{
  return bmi270_get_sensor_data(accel_data, gyro_data);
}

int16_t app_imu_get_accel_module_rms(void)
{
  struct accelerometer_sensor_data_t accel_data;
  app_imu_get_accel_gyro_rms(&accel_data, NULL);
  return sqrt(accel_data.x * accel_data.x + accel_data.y * accel_data.y + accel_data.z * accel_data.z);
}

int16_t app_imu_get_gyro_module_rms(void)
{
  struct gyroscope_sensor_data_t gyro_data;
  app_imu_get_accel_gyro_rms(NULL, &gyro_data);
  return sqrt(gyro_data.x * gyro_data.x + gyro_data.y * gyro_data.y + gyro_data.z * gyro_data.z);
}