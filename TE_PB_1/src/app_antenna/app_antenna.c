/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @author Étienne Machabée (etienne@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) Lynkz Instruments Inc 2023
* 
*/
#include "app_antenna.h"

#include "app_peripherals.h"
#include "app.h"
#include "app_settings.h"
#include "app_i2c.h"
#include "ldc1614.h"
#include "tsys02d.h"

/** Definitions**/
#define EMPTY_SERIAL_NUM (0xFFFFFFFFFFFFFFFF)

// LDC1614 configuration
// Important configuration notes: To set the current manually, AUTO_AMP_DIS and RP_OVERRIDE_EN must be set to 0b1
#define LDC1614_CONFIG_ACTIVE_CHAN_DEFAULT (0b00)       // CH0, but unused
#define LDC1614_CONFIG_SLEEP_MODE_EN_DEFAULT (0b1)      // By default sleep
#define LDC1614_CONFIG_RP_OVERRIDE_EN_DEFAULT (0b1)     // Override Rp to control current
#define LDC1614_CONFIG_SENSOR_ACTIVATE_SEL_DEFAULT (0b1)// Use low power sensor activation
#define LDC1614_CONFIG_AUTO_AMP_DIS_DEFAULT (0b1)       // Disable automatic amplitude correction.
#define LDC1614_CONFIG_REF_CLK_SRC_DEFAULT (0b1)        // External 40 MHz clock as CLK source
#define LDC1614_CONFIG_RESERVED1_DEFAULT (0b0)
#define LDC1614_CONFIG_INTB_DIS_DEFAULT (0b0)           // Disable the INTB pin (unconnected in our app)
#define LDC1614_CONFIG_HIGH_CURRENT_DRV_DEFAULT (0b0)   // Multi-channel is used, disable CH0 high current drive
#define LDC1614_CONFIG_RESERVED0_DEFAULT (0b000001)

// LDC1614 error configuration
// Config tl;dr: Report every error possible
#define LDC1614_ERRCONFIG_UR_ERR2OUT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_OR_ERR2OUT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_WD_ERR2OUT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_AH_ERR2OUT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_AL_ERR2OUT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_RESERVED1_DEFAULT (0b000)
#define LDC1614_ERRCONFIG_UR_ERR2INT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_OR_ERR2INT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_WD_ERR2INT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_AH_ERR2INT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_AL_ERR2INT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_ZC_ERR2INT_DEFAULT (0b1)
#define LDC1614_ERRCONFIG_RESERVED0_DEFAULT (0b0)
#define LDC1614_ERRCONFIG_DRDY_2INT_DEFAULT (0b1)

// LDC1614 mux configuration
#define LDC1614_MUXCONFIG_DEGLITCH_DEFAULT (0b101)          // 10 MHz
#define LDC1614_MUXCONFIG_RESERVED0_DEFAULT (0b0001000001)
#define LDC1614_MUXCONFIG_RR_SEQUENCE_DEFAULT (0b00)        // Sample CH0 and CH1
#define LDC1614_MUXCONFIG_AUTOSCAN_EN_DEFAULT (0b1)         // Enable autoscan

// LDC1614 individual registers configuration
#define LDC1614_REG_OFFSET0_DEFAULT (0)
#define LDC1614_REG_OFFSET1_DEFAULT (0)
#define LDC1614_REG_SETTLECOUNT0_DEFAULT (0x0040)     // Determined experimentally
#define LDC1614_REG_SETTLECOUNT1_DEFAULT (0x0200)     // Determined experimentally
#define LDC1614_REG_CLOCK_DIVIDERS0_FIN_DEFAULT (1)   // Frequency will be below ~8 MHz
#define LDC1614_REG_CLOCK_DIVIDERS0_FREF_DEFAULT (8)  // Divide clock for more precise measurement
#define LDC1614_REG_CLOCK_DIVIDERS1_FIN_DEFAULT (2)   // Frequency can be above ~8 MHz
#define LDC1614_REG_CLOCK_DIVIDERS1_FREF_DEFAULT (1)  // Frequency changes too much to divide clock
#define LDC1614_REG_DRIVE_CURRENT0_DEFAULT (19)       // Determined experimentally
#define LDC1614_REG_DRIVE_CURRENT1_DEFAULT (22)       // Determined experimentally

/** Global variables **/
static bool initialized = false;

// Device Handles
static LDC1614_dev_t ldc1614;
static struct tsys02d_dev tsys02d;

/** Private function prototypes **/
static bool init_tsys02d(void);
static bool InitLDC1614(uint8_t bitmask);
static void interface_tsys02d(struct tsys02d_dev *dev);
static void InterfaceLDC1614(LDC1614_dev_t *dev);
static void log_ldc_status(const LDC1614_status_t status);

/** Public functions **/
bool app_antenna_init(uint8_t bitmask){

  bool init_rslt = true;

  // Power the Antenna Assembly
  app_hdw_pwr_antenna(true);
  nrf_delay_ms(50);

  // Init tsys02d
  init_rslt &= init_tsys02d();

  // Init ldc1614
  init_rslt &= InitLDC1614(bitmask);

  initialized = init_rslt;

  if (!initialized) {
    NRF_LOG_INFO("[APP_ANTENNA] Error initializing the antenna");
    // Protect from failures by shutting down antenna
    app_hdw_pwr_antenna(false);
  } else {
    NRF_LOG_INFO("[APP_ANTENNA] Antenna initialized");
  }

  return init_rslt;
}

bool app_antenna_sleep(void) {
  bool status = true;

  // Power ON the antenna to confirm it's ON
  app_hdw_pwr_antenna(true);
  nrf_delay_ms(25);

  status &= LDC1614_Sleep();

  NRF_LOG_DEBUG("Sleeping the antenna, success: %d", status);

  return status;
}

void app_antenna_uninit(void)
{
  app_i2c_uninit();
  initialized = false;
}

bool app_antenna_wake_up(void) {
  bool status = true;

  // Power ON the antenna to confirm it's ON
  app_hdw_pwr_antenna(true);
  nrf_delay_ms(25);

  status &= LDC1614_WakeUp();

  return status;
}

bool app_antenna_get_frequency(uint8_t channel, uint32_t* value, uint8_t* error_mask){
  if (NULL == value || NULL == error_mask || channel > LDC1614_CHANNEL_NUM) {
    return false;
  }

  if(LDC1614_GetChannelResult(channel, value, error_mask) != LDC1614_ERR_NO_ERROR) return false;

  return true;
}

bool app_antenna_get_temperature(uint16_t* temperature){

  uint16_t value = 0;

  if(tsys02d_conversion_and_read_adc(&value) != tsys02d_status_ok) return false;

  *temperature = value;
  return true;
}

/** Private functions **/

static bool init_tsys02d(void)
{    
  uint64_t serial_number = EMPTY_SERIAL_NUM;

  // Initialize Interface with tsys02d Driver
  interface_tsys02d(&tsys02d);
  tsys02d_init();

  // Init TWI
  if(app_i2c_init()){
    // Read serial number to validate initialization
    tsys02d_read_serial_number(&serial_number);

    //NRF_LOG_INFO("TSYS02D S/N: 0x%016X");
    
    if(serial_number != EMPTY_SERIAL_NUM){
      return true;
    }
  }
  return false;
}

static bool InitLDC1614(uint8_t bitmask)
{
  uint16_t mfg_id = 0xFFFF;

  if (bitmask > 0x03) {
    return false; // Error, invalid bitmask
  }

  // Configure LDC1614 Driver
  if (bitmask == 0x01 || bitmask == 0x02) {
    ldc1614.muxconfig.fields.autoscan_en = 0;
    if (bitmask == 0x01) {
      ldc1614.config.fields.active_chan = 0b00;
    } else {
      ldc1614.config.fields.active_chan = 0b01;
    }
  } else {
    ldc1614.muxconfig.fields.autoscan_en = 1;
    ldc1614.muxconfig.fields.rr_sequence = 0b00;
  }

  InterfaceLDC1614(&ldc1614);

  // Init TWI
  if (!app_i2c_init()) {
    return false;
  }

  // Get Sensor Information to validate, then configure the sensor
  if (LDC1614_read_sensor_infomation(&mfg_id) != LDC1614_ERR_NO_ERROR) {
    NRF_LOG_INFO("[APP_ANTENNA] Error communicating to LDC1614");
    return false;
  }

  if (mfg_id != LDC1614_MFG_ID) {
    NRF_LOG_INFO("[APP_ANTENNA] Invalid LDC1614 MFG ID");
    return false;
  }

  if (LDC1614_ResetSensor() != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] Error with LDC1614 reset");
    return false;
  }

  // Sleep the device to fully configure the rest of the device
  if (!LDC1614_Sleep()){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 SLEEP");
    return false;
  }

  if (LDC1614_SetSensorConfig() != LDC1614_ERR_NO_ERROR){
   NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 CONFIG SETTING");
   return false;
  }

  if (LDC1614_SetConversionTime(LDC1614_CHANNEL_0, LDC1614_CONVERSION_TIME_INTERVAL) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 CONVERSION TIME 0");
    return false;
  }

  if (LDC1614_SetConversionTime(LDC1614_CHANNEL_1, LDC1614_CONVERSION_TIME_INTERVAL) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 CONVERSION TIME 1");
    return false;
  }

  if (LDC1614_SetConversionOffset(LDC1614_CHANNEL_0, LDC1614_REG_OFFSET0_DEFAULT) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 OFFSET 0");
    return false;
  }

  if (LDC1614_SetConversionOffset(LDC1614_CHANNEL_1, LDC1614_REG_OFFSET1_DEFAULT) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 OFFSET 1");
    return false;
  }

  if (LDC1614_SetSettlecount(LDC1614_CHANNEL_0, app_settings_get_ch0_settle_count()) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 STABILIZE TIME 0");
    return false;
  }

  if (LDC1614_SetSettlecount(LDC1614_CHANNEL_1, app_settings_get_ch1_settle_count()) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 STABILIZE TIME 1");
    return false;
  }

  if (LDC1614_SetClockDividers(LDC1614_CHANNEL_0, LDC1614_REG_CLOCK_DIVIDERS0_FIN_DEFAULT,
      LDC1614_REG_CLOCK_DIVIDERS0_FREF_DEFAULT ) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 CLOCK 0");
    return false;
  }

  if (LDC1614_SetClockDividers(LDC1614_CHANNEL_1, LDC1614_REG_CLOCK_DIVIDERS1_FIN_DEFAULT,
      LDC1614_REG_CLOCK_DIVIDERS1_FREF_DEFAULT ) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 CLOCK 1");
    return false;
  }

  if (LDC1614_SetMuxConfig() != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 MUX CONFIG");
    return false;
  }

  if (LDC1614_SetErrorConfig() != LDC1614_ERR_NO_ERROR) {
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 ERR CONFIG");
    return false;
  }

  if (LDC1614_SetDriveCurrent(LDC1614_CHANNEL_0, app_settings_get_ch0_drive_current()) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 CURRENT 0");
    return false;
  }

  if (LDC1614_SetDriveCurrent(LDC1614_CHANNEL_1, app_settings_get_ch1_drive_current()) != LDC1614_ERR_NO_ERROR){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 CURRENT 1");
    return false;
  }

  if (!LDC1614_WakeUp()){
    NRF_LOG_INFO("[APP_ANTENNA] ERROR WITH LDC1614 AWAKE");
    return false;
  }
  return true;
}

bool read_tsys02d(struct i2c_master_packet *const packet)
{
  uint16_t address    = packet->address;
  uint8_t *data     = packet->data;
  uint16_t len      = packet->data_length;

  app_i2c_xfer_result_t result = app_i2c_rx(address, data, len);
  if (result != APP_I2C_XFER_RESULT_SUCCESS) {
    return false;
  }

  return true;
}

bool write_tsys02d(struct i2c_master_packet *const packet){

  uint16_t address    = packet->address;
  uint8_t *data     = packet->data;
  uint16_t len      = packet->data_length;

  app_i2c_xfer_result_t result = app_i2c_tx(address, data, len);
  if (result != APP_I2C_XFER_RESULT_SUCCESS) {
    return false;
  }

  return true;
}

bool read_ldc1614(struct LDC1614_i2c_packet *const packet)
{
  uint16_t address    = packet->address;
  uint8_t *data     = packet->data;
  uint16_t len      = packet->data_length;

  app_i2c_xfer_result_t result = app_i2c_rx(address, data, len);
  if (result != APP_I2C_XFER_RESULT_SUCCESS) {
    return false;
  }

  return true;
}

bool write_ldc1614(struct LDC1614_i2c_packet *const packet)
{
  uint16_t address    = packet->address;
  uint8_t *data     = packet->data;
  uint16_t len      = packet->data_length;

  app_i2c_xfer_result_t result = app_i2c_tx(address, data, len);
  if (result != APP_I2C_XFER_RESULT_SUCCESS) {
    return false;
  }

  return true;
}

// Wrapper to make it work with the drivers
bool delay_ms(uint32_t time)
{
  nrf_delay_ms(time);
  return true;
}

// Create the interface with the driver
static void interface_tsys02d(struct tsys02d_dev *dev)
{
  dev->initialization = app_i2c_init;
  dev->read = read_tsys02d;
  dev->write = write_tsys02d;
  dev->write_no_stop = write_tsys02d;
  dev->delay = delay_ms;

  tsys02d_setup_interface(dev);
}

static void InterfaceLDC1614(LDC1614_dev_t *dev)
{
  dev->initialization = app_i2c_init;
  dev->read = read_ldc1614;
  dev->write = write_ldc1614;
  dev->delay = delay_ms;

  // Create sensor config, set it to the default value for the application
  dev->config.fields.active_chan = LDC1614_CONFIG_ACTIVE_CHAN_DEFAULT;
  dev->config.fields.sleep_mode_en = LDC1614_CONFIG_SLEEP_MODE_EN_DEFAULT;
  dev->config.fields.rp_override_en = LDC1614_CONFIG_RP_OVERRIDE_EN_DEFAULT;
  dev->config.fields.sensor_activate_sel = LDC1614_CONFIG_SENSOR_ACTIVATE_SEL_DEFAULT;
  dev->config.fields.auto_amp_dis = LDC1614_CONFIG_AUTO_AMP_DIS_DEFAULT;
  dev->config.fields.ref_clk_src = LDC1614_CONFIG_REF_CLK_SRC_DEFAULT;
  dev->config.fields.reserved1 = LDC1614_CONFIG_RESERVED1_DEFAULT;
  dev->config.fields.intb_dis = LDC1614_CONFIG_INTB_DIS_DEFAULT;
  dev->config.fields.high_current_drv = LDC1614_CONFIG_HIGH_CURRENT_DRV_DEFAULT;
  dev->config.fields.reserved0 = LDC1614_CONFIG_RESERVED0_DEFAULT;

  dev->errconfig.fields.ur_err2out = LDC1614_ERRCONFIG_UR_ERR2OUT_DEFAULT;
  dev->errconfig.fields.or_err2out = LDC1614_ERRCONFIG_OR_ERR2OUT_DEFAULT;
  dev->errconfig.fields.wd_err2out = LDC1614_ERRCONFIG_WD_ERR2OUT_DEFAULT;
  dev->errconfig.fields.ah_err2out = LDC1614_ERRCONFIG_AH_ERR2OUT_DEFAULT;
  dev->errconfig.fields.al_err2out = LDC1614_ERRCONFIG_AL_ERR2OUT_DEFAULT;
  dev->errconfig.fields.reserved1 = LDC1614_ERRCONFIG_RESERVED1_DEFAULT;
  dev->errconfig.fields.ur_err2int = LDC1614_ERRCONFIG_UR_ERR2INT_DEFAULT;
  dev->errconfig.fields.or_err2int = LDC1614_ERRCONFIG_OR_ERR2INT_DEFAULT;
  dev->errconfig.fields.wd_err2int = LDC1614_ERRCONFIG_WD_ERR2INT_DEFAULT;
  dev->errconfig.fields.ah_err2int = LDC1614_ERRCONFIG_AH_ERR2INT_DEFAULT;
  dev->errconfig.fields.al_err2int = LDC1614_ERRCONFIG_AL_ERR2INT_DEFAULT;
  dev->errconfig.fields.zc_err2int = LDC1614_ERRCONFIG_ZC_ERR2INT_DEFAULT;
  dev->errconfig.fields.reserved0 = LDC1614_ERRCONFIG_RESERVED0_DEFAULT;
  dev->errconfig.fields.drdy_2int = LDC1614_ERRCONFIG_DRDY_2INT_DEFAULT;

  dev->muxconfig.fields.deglitch = LDC1614_MUXCONFIG_DEGLITCH_DEFAULT;
  dev->muxconfig.fields.reserved0 = LDC1614_MUXCONFIG_RESERVED0_DEFAULT;
  dev->muxconfig.fields.rr_sequence = LDC1614_MUXCONFIG_RR_SEQUENCE_DEFAULT;
  dev->muxconfig.fields.autoscan_en = LDC1614_MUXCONFIG_AUTOSCAN_EN_DEFAULT;

  if (LDC1614_Setup(dev) != LDC1614_ERR_NO_ERROR) {
    NRF_LOG_INFO("[APP_ANTENNA] Error in the LDC1614 device structure");
  } else {
    NRF_LOG_INFO("[APP_ANTENNA] Initialized LDC1614 device structure");
  }
}

static void log_ldc_status(const LDC1614_status_t status) {
  NRF_LOG_INFO("LDC1614 status:");
  NRF_LOG_INFO("Error Channel: %d", status.fields.err_chan);
  NRF_LOG_INFO("Conversion Under-range Error: %d", status.fields.err_ur);
  NRF_LOG_INFO("Conversion Over-range Error: %d", status.fields.err_or);
  NRF_LOG_INFO("Watchdog Timeout Error: %d", status.fields.err_wd);
  NRF_LOG_INFO("Sensor Amplitude High Error: %d", status.fields.err_ahe);
  NRF_LOG_INFO("Sensor Amplitude Low Error: %d", status.fields.err_ale);
  NRF_LOG_INFO("Zero Count Error: %d", status.fields.err_zc);
  NRF_LOG_INFO("Data Ready Flag: %d", status.fields.drdy);
  NRF_LOG_INFO("Channel 0 Unread Conversion: %d", status.fields.unreadconv0);
  NRF_LOG_INFO("Channel 1 Unread Conversion: %d", status.fields.unreadconv1);
  NRF_LOG_INFO("Channel 2 Unread Conversion: %d", status.fields.unreadconv2);
  NRF_LOG_INFO("Channel 3 Unread Conversion: %d", status.fields.unreadconv3);
}
