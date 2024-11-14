/**
* @author Alexandre Desgagné (alexd@lynkz.ca)
* @brief 
* @version 1.0
* @date 2023-04-15
* 
* @copyright Copyright (c) 2023
* 
*/

#include "app_hardware.h"

#include "app_peripherals.h"
#include "app_settings.h"

#define RTC_FREQUENCY_HZ 100 // give a period of 1 sec to RTC
#define RTC_PRESACLER (32768 / RTC_FREQUENCY_HZ) - 1

bool nfc_minimal_initialization_done = false;
nrf_drv_wdt_channel_id m_channel_id;
const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(2); // Declaring an instance of nrf_drv_rtc for RTC0.

/**
 * @brief Main function to deep sleep the device.
 */
static void device_deep_sleep(void);

/**
 * @brief Main function to initialize logs.
 */
static void log_init(void);

/**
 * @brief Main function to initialize 
 * what's needed for NFC implementation.
 */
static void init_hardware_nfc_minimal(void);

/**
 * @brief Main function to initialize watch dog.
 */
static void wdt_init(void);

/**
 * @brief Main function to initialize timers.
 */
static void timers_init(void);

/**
 * @brief Main function to initialize RTC.
 */
static void rtc_init(void);

/**
 * @brief Main function to initialize gpios.
 */
static void gpio_init(void);

bool app_hdw_init(void)
{
  // Init logging system.
  log_init();

  NRF_LOG_INFO("SMARTLINERS MAINBOARD FW %s STARTED!", FW_VERSION);

  // Minimal hardware for nfc. Clock and power management.
  init_hardware_nfc_minimal();
  
  // Configure test board detection pin.
  nrf_gpio_cfg_input(TB_DETECT_PIN, NRF_GPIO_PIN_PULLDOWN);
  

  // Get the reset reason
  NRF5_UTILS_GetResetReasons();

  // Init watchdog
  wdt_init();

  // Init timers
  timers_init();
  
  // Init RTC
  rtc_init();

  // Init GPIOs
  gpio_init();

  return true;
}

static void device_deep_sleep(void)
{
  // Configure blue LED  
  nrf_gpio_cfg_output(BLUE_LED);
  
  // Blink blue LED.
  for(uint8_t i = 0; i < 3; ++i){
    app_hdw_set_blue_led(true);
    nrf_delay_ms(50);
    app_hdw_set_blue_led(false);
    nrf_delay_ms(50);
  }

  // Empty the LOG Buffer
  while( NRF_LOG_PROCESS() );

  // Set device to System OFF mode
  nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
}

// LOG
static void log_init(void)
{
  ret_code_t err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);

  NRF_LOG_DEFAULT_BACKENDS_INIT();
}

// NFC
static void init_hardware_nfc_minimal(void)
{
  if(nfc_minimal_initialization_done) return ;

  // Power management
  APP_ERROR_CHECK( nrf_pwr_mgmt_init() );

  // Clock
  APP_ERROR_CHECK( nrf_drv_clock_init() );
  nrf_drv_clock_lfclk_request(NULL);

  nfc_minimal_initialization_done = true;
}

// WDT
static void wdt_event_handler(void)
{
  // The max amount of time we can spend in WDT interrupt is two cycles of 32768[Hz] clock - after that, reset occurs
}

static void wdt_init(void)
{
  uint32_t err_code = 0;

  // Configure WDT.
  nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;
  err_code = nrf_drv_wdt_init(&config, wdt_event_handler);
  APP_ERROR_CHECK(err_code);
  err_code = nrf_drv_wdt_channel_alloc(&m_channel_id);
  APP_ERROR_CHECK(err_code);
  nrf_drv_wdt_enable();
}

void app_hdw_wdt_kick(void)
{
  nrf_drv_wdt_channel_feed(m_channel_id); // Kick watchdog.
}

// TIMERS
static void timers_init(void)
{
  // Initialize timer module, making it use the scheduler
  ret_code_t err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
}

// RTC
void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
  if (int_type == NRF_DRV_RTC_INT_COMPARE0)
  {    
    nrf_drv_rtc_int_enable(&rtc, NRF_RTC_INT_COMPARE0_MASK); 
    nrf_drv_rtc_counter_clear(&rtc);
    SCH_Tick_Handler();
  }
}

static void rtc_init(void)
{
  uint32_t err_code;

  //Initialize RTC instance
  nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
  config.prescaler = RTC_PRESACLER;
  err_code = nrf_drv_rtc_init(&rtc, &config, rtc_handler);
  APP_ERROR_CHECK(err_code);

  //Set compare channel to trigger interrupt after COMPARE_COUNTERTIME seconds
  err_code = nrf_drv_rtc_cc_set(&rtc, 0, WAKE_UP_TIME_SEC * RTC_FREQUENCY_HZ, true);
  APP_ERROR_CHECK(err_code);

  //Power on RTC instance
  nrf_drv_rtc_enable(&rtc);
}

// GPIOs
static void gpio_init(void)
{
  // LEDs
  nrf_gpio_cfg(RED_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(GREEN_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(BLUE_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  
  // BMI
  nrf_gpio_cfg(IMU_INT_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  // FLASH
  // nrf_gpio_cfg_output(PWR_FLASH_PIN);
  nrf_gpio_cfg(PWR_FLASH_BMI_PIN, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_H0H1, NRF_GPIO_PIN_NOSENSE);

  // LORA radio
  // nrf_gpio_cfg_output(PWR_LORA_PIN);
  nrf_gpio_cfg(PWR_LORA_PIN, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_H0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(UART_RX_PIN_NUMBER, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_H0H1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg_output(LORA_RST_PIN);

  // ANTENNA assembly
  nrf_gpio_cfg(PWR_ANTENNA_PIN, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_H0H1, NRF_GPIO_PIN_NOSENSE);

  // TestBoard Detection Pin. Will read 1 when testboard is present.
  nrf_gpio_cfg_input(TB_DETECT_PIN, NRF_GPIO_PIN_PULLDOWN);

  // Set Default GPIO States
  nrf_gpio_pin_set(LORA_RST_PIN);
  
  app_hdw_set_leds(false, true, false); // Turn on green LED during initialization.
  app_hdw_pwr_lora(true);
  app_hdw_pwr_flash_bmi(false);
  app_hdw_pwr_antenna(false);
}

void app_hdw_set_leds(bool red, bool green, bool blue)
{
  app_hdw_set_red_led(red);
  app_hdw_set_green_led(green);
  // Don't touch the blue LED when user id connected.
  // This is the LED used to show a BLE connection with the device.
  if (!is_ble_user_connected()){
    app_hdw_set_blue_led(blue);
  }
}

void app_hdw_set_red_led(bool on)
{
  if (on){
    nrf_gpio_pin_clear(RED_LED);
  }
  else{
    nrf_gpio_pin_set(RED_LED);
  }
}

void app_hdw_set_green_led(bool on)
{
  if (on){
    nrf_gpio_pin_clear(GREEN_LED);
  }
  else{
    nrf_gpio_pin_set(GREEN_LED);
  }
}

void app_hdw_set_blue_led(bool on)
{
  if (on){
    nrf_gpio_pin_clear(BLUE_LED);
  }
  else{
    nrf_gpio_pin_set(BLUE_LED);
  }
}

void app_hdw_pwr_antenna(bool state)
{
  if(state){  
    nrf_gpio_pin_clear(PWR_ANTENNA_PIN);
  }
  else{
    nrf_gpio_pin_set(PWR_ANTENNA_PIN);
  }
}

void app_hdw_pwr_lora(bool state)
{
  if(state){
    nrf_gpio_pin_clear(PWR_LORA_PIN);
    nrf_delay_ms(100);
  }
  else {
    nrf_gpio_pin_set(PWR_LORA_PIN);
  }
}

void app_hdw_pwr_flash_bmi(bool state)
{
  if(state){
    nrf_gpio_pin_clear(PWR_FLASH_BMI_PIN);
  }
  else{
    nrf_gpio_pin_set(PWR_FLASH_BMI_PIN);
  }
}

void app_hdw_disconnect_lora_uart(void)
{
  nrf_gpio_cfg_output(UART_TX_PIN_NUMBER);
  nrf_gpio_cfg_output(UART_RX_PIN_NUMBER);
  nrf_gpio_pin_set(UART_TX_PIN_NUMBER);
  nrf_gpio_pin_set(UART_RX_PIN_NUMBER);
}

void app_hdw_disconnect_spi(void)
{
  nrf_gpio_cfg(SPIM1_MISO_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SPIM1_MOSI_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SPIM1_SCK_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SPIM1_CSB_FLASH_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SPIM1_CSB_IMU_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
}

void app_hdw_disconnect_i2c(void)
{
  nrf_gpio_cfg(I2CM0_SDA_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(I2CM0_SCL_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
}

void app_hdw_gpio_low_power(void)
{
  app_hdw_set_leds(false, false, false);

  app_hdw_pwr_flash_bmi(false);
  app_hdw_pwr_antenna(false);

  app_hdw_disconnect_spi();
  app_hdw_disconnect_i2c();
  app_hdw_disconnect_lora_uart();

  nrf_gpio_cfg(IMU_INT_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
}

bool app_hdw_is_on_test_board(void)
{
  return (bool)nrf_gpio_pin_read(TB_DETECT_PIN); 
}

void app_hdw_disconnect_test_board_detect(void)
{
  nrf_gpio_cfg(TB_DETECT_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(TB_RX_PIN_NUMBER, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(TB_TX_PIN_NUMBER, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
}