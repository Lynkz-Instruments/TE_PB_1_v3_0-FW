/**
* @author Lucas Bonenfant (lucas@lynkz.ca)
* @brief 
* @version 1.0
* @date 2024-11-13
* 
* @copyright Copyright (c) Lynkz Instruments Inc, Amos 2024
* 
*/

#include "app_hardware.h"

#include "app_peripherals.h"
#include "app_settings.h"

#define RTC_FREQUENCY_HZ 100 // give a period of 1 sec to RTC
#define RTC_PRESACLER (32768 / RTC_FREQUENCY_HZ) - 1

#define NB_MODE 4
#define NB_UART_CONF 3

#define V_BAT_TRHL 3.0

uint8_t mode = 0;
uint8_t uart_conf = 0;

bool mode_BTN_last_state = false;
bool UART_BTN_last_state = false;



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
  nrf_gpio_cfg(INT_STCO_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(INT_BV_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(MUX1_UART_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(MUX2_UART_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(LOW_BAT_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  // Buttons (À VÉRIFIER)
  nrf_gpio_cfg(UART_SELECTOR_BTN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(MODE_SELECTOR_BTN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  // Power SENS (À VÉRIFIER)
  nrf_gpio_cfg(TAG_PWR_SENS, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(V_BAT_SENS, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  // STARTCO SENS (À VÉRIFIER)
  nrf_gpio_cfg(STCO_OK, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(STCO_OPEN_Z, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(STCO_SHORT_Z, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  // Analog Switches 
  nrf_gpio_cfg(SW1, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SW2, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SW3, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SW4_5, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SW6, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  // TAG power
  nrf_gpio_cfg(TAG_PWR, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  // UART config
}

void app_hdw_select_mode()
{
  switch (mode)
  {
  case 1:
    app_hdw_set_INT_STCO_led(false);
    app_hdw_set_INT_BV_led(false);

    app_hdw_set_analog_switch1(true);
    app_hdw_set_analog_switch2(false);
    app_hdw_set_analog_switch3(false);
    app_hdw_set_analog_switch4_5(true);        
    app_hdw_set_analog_switch6(false);
    break;
      
  case 2:
    app_hdw_set_INT_STCO_led(true);
    app_hdw_set_INT_BV_led(false);

    app_hdw_set_analog_switch1(true);
    app_hdw_set_analog_switch2(false);
    app_hdw_set_analog_switch3(false);
    app_hdw_set_analog_switch4_5(false);        
    app_hdw_set_analog_switch6(false);
    break;

  case 3:
    app_hdw_set_INT_STCO_led(true);
    app_hdw_set_INT_BV_led(true);

    app_hdw_set_analog_switch1(true);
    app_hdw_set_analog_switch2(false);
    app_hdw_set_analog_switch3(true);
    app_hdw_set_analog_switch4_5(true);        
    app_hdw_set_analog_switch6(true);

    break;

  case 4:
    app_hdw_set_INT_STCO_led(false);
    app_hdw_set_INT_BV_led(true);

    app_hdw_set_analog_switch1(false);
    app_hdw_set_analog_switch2(true);
    app_hdw_set_analog_switch3(false);
    app_hdw_set_analog_switch4_5(true);        
    app_hdw_set_analog_switch6(true);
    break;

  default:
    break;
  }

}

void app_hdw_select_UART()
{
  switch (uart_conf)
  {
  case 1:
    app_hdw_set_UART1_led(false);
    app_hdw_set_UART2_led(false);
    break;
  case 2:
    app_hdw_set_UART1_led(true);
    app_hdw_set_UART2_led(false);
    break;
  case 3:
    app_hdw_set_UART1_led(false);
    app_hdw_set_UART2_led(true);
    break;

  
  default:
      break;
  }

}

void app_hdw_set_analog_switch1(bool on)
{
  if (on){
    nrf_gpio_pin_clear(SW1);
  }
  else{
    nrf_gpio_pin_set(SW1);
  }
}

void app_hdw_set_analog_switch2(bool on)
{
  if (on){
    nrf_gpio_pin_clear(SW2);
  }
  else{
    nrf_gpio_pin_set(SW2);
  }
}

void app_hdw_set_analog_switch3(bool on)
{
  if (on){
    nrf_gpio_pin_clear(SW3);
  }
  else{
    nrf_gpio_pin_set(SW3);
  }
}

void app_hdw_set_analog_switch4_5(bool on)
{
  if (on){
    nrf_gpio_pin_clear(SW4_5);
  }
  else{
    nrf_gpio_pin_set(SW4_5);
  }
}

void app_hdw_set_analog_switch6(bool on)
{
  if (on){
    nrf_gpio_pin_clear(SW6);
  }
  else{
    nrf_gpio_pin_set(SW6);
  }
}

void app_hdw_set_INT_STCO_led(bool on)
{
  if (on){
    nrf_gpio_pin_clear(INT_STCO_LED);
  }
  else{
    nrf_gpio_pin_set(INT_STCO_LED);
  }
}

void app_hdw_set_INT_BV_led(bool on)
{
  if (on){
    nrf_gpio_pin_clear(INT_BV_LED);
  }
  else{
    nrf_gpio_pin_set(INT_BV_LED);
  }
}

void app_hdw_set_UART1_led(bool on)
{
  if (on){
    nrf_gpio_pin_clear(MUX1_UART_LED);
  }
  else{
    nrf_gpio_pin_set(MUX1_UART_LED);
  }
}

void app_hdw_set_UART2_led(bool on)
{
  if (on){
    nrf_gpio_pin_clear(MUX2_UART_LED);
  }
  else{
    nrf_gpio_pin_set(MUX2_UART_LED);
  }
}

void app_hdw_set_low_bat_led(bool on)
{
  if (on){
    nrf_gpio_pin_clear(LOW_BAT_LED);
  }
  else{
    nrf_gpio_pin_set(LOW_BAT_LED);
  }
}

void app_hdw_set_TAG_pwr(bool on)
{
  if (on){
    nrf_gpio_pin_clear(TAG_PWR);
  }
  else{
    nrf_gpio_pin_set(TAG_PWR);
  }
}

void app_hdw_read_mode_BTN()
{
  bool state = !(bool)nrf_gpio_pin_read(MODE_SELECTOR_BTN);
  if (state &&  state != mode_BTN_last_state){
    mode++;
    if (mode > NB_MODE)
    {
      mode = 0;
    }
    app_hdw_select_mode();
  }
  mode_BTN_last_state = state;
}

void app_hdw_read_UART_BTN()
{
  bool state = !(bool)nrf_gpio_pin_read(UART_SELECTOR_BTN);
  if (state &&  state != UART_BTN_last_state){
    uart_conf++;
    if (uart_conf > NB_UART_CONF){
      uart_conf = 0;
    }
    app_hdw_select_UART();
  }
  UART_BTN_last_state = state;
}

void app_hdw_read_V_BAT()
{
  uint8_t voltage = nrf_gpio_pin_read(V_BAT_SENS);
  if (voltage < V_BAT_TRHL){
    app_hdw_set_low_bat_led(true);
  }
  else {
    app_hdw_set_low_bat_led(false);
  }
}

void app_hdw_detect_TAG()
{
  if ((bool)nrf_gpio_pin_read(STCO_OK)){
    nrf_delay_ms(1000);
    if (!(bool)nrf_gpio_pin_read(TAG_PWR_SENS)){
      app_hdw_set_TAG_pwr(true);
    }
      app_hdw_set_TAG_pwr(false);
  }
}
