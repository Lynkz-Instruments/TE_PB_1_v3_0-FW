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
#include "app_saadc.h"
#include "app_ppi.h"
#include "app_uart_module.h"

#define RTC_FREQUENCY_HZ 100 // give a period of 1 sec to RTC
#define RTC_PRESACLER (32768 / RTC_FREQUENCY_HZ) - 1

#define NB_MODE 4
#define NB_UART_CONF 3

#define V_BAT_TRHL 150

uint8_t mode = 0;
uint8_t uart_conf = 0;
bool uart_ble = true;

static bool interrupt_initialized = false;


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

void buttons_interrupt_init(void);

void UART_button_interrupt_init(void);

void mode_button_interrupt_init(void);

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

  // Init SAADC
  app_saadc_init();

  // Init GPIOs
  gpio_init();

  // Initial condition
  mode = 0;
  uart_conf = 0;
  app_hdw_select_UART();
  app_hdw_select_mode();

  buttons_interrupt_init();

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
  NRF_LOG_INFO("LED_PINS");
  // LEDs
  nrf_gpio_cfg(INT_STCO_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(INT_BV_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(MUX1_UART_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(MUX2_UART_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(LOW_BAT_LED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  NRF_LOG_INFO("POWER_SENS");
  // Power SENS (À VÉRIFIER)
  nrf_gpio_cfg(TAG_PWR_SENS, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  //nrf_gpio_cfg(V_BAT_SENS, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
 
  NRF_LOG_INFO("STCO_PINS");
  // STARTCO SENS (À VÉRIFIER)
  nrf_gpio_cfg(STCO_OK_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(STCO_OPEN_Z_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(STCO_SHORT_Z_PIN, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  NRF_LOG_INFO("SWITCHES_PINS");
  // Analog Switches 
  nrf_gpio_cfg(SW1, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SW2, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SW3, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SW4_5, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_cfg(SW6, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

  NRF_LOG_INFO("TAG_PIN");
  // TAG power
  nrf_gpio_cfg(TAG_PWR, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

}

void app_hdw_select_mode()
{
  switch (mode)
  {
  case 0:
    app_hdw_set_INT_STCO_led(false);
    app_hdw_set_INT_BV_led(false);

    app_hdw_set_analog_switch1(true);
    app_hdw_set_analog_switch2(false);
    app_hdw_set_analog_switch3(false);
    app_hdw_set_analog_switch4_5(true);        
    app_hdw_set_analog_switch6(false);
    break;
      
  case 1:
    app_hdw_set_INT_STCO_led(true);
    app_hdw_set_INT_BV_led(false);

    app_hdw_set_analog_switch1(true);
    app_hdw_set_analog_switch2(false);
    app_hdw_set_analog_switch3(false);
    app_hdw_set_analog_switch4_5(false);        
    app_hdw_set_analog_switch6(false);
    break;

  case 2:
    app_hdw_set_INT_STCO_led(true);
    app_hdw_set_INT_BV_led(true);

    app_hdw_set_analog_switch1(true);
    app_hdw_set_analog_switch2(false);
    app_hdw_set_analog_switch3(true);
    app_hdw_set_analog_switch4_5(true);        
    app_hdw_set_analog_switch6(true);

    break;

  case 3:
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
  
  NRF_LOG_INFO("MODE : %d", mode);

}

void app_hdw_select_UART()
{

  ret_code_t err_code;

  if (!uart_ble) {
    switch (uart_conf)
    { 
    case 0:
      app_hdw_set_UART1_led(false);
      app_hdw_set_UART2_led(false);
      app_ppi_free_channel(0, BV_RX_PIN_NUMBER, UART_TX_PIN_NUMBER);

     
      err_code = app_uart_init_PB();
      APP_ERROR_CHECK(err_code);

    break;

    case 1:
      app_hdw_set_UART1_led(true);
      app_hdw_set_UART2_led(false);

      err_code = app_uart_module_uninit();
      APP_ERROR_CHECK(err_code);
      app_ppi_configure_channel(0, UART_RX_PIN_NUMBER, TAG_TX_PIN_NUMBER);
      app_ppi_configure_channel(1, TAG_RX_PIN_NUMBER, UART_TX_PIN_NUMBER);

    break;

    case 2:
      app_hdw_set_UART1_led(false);
      app_hdw_set_UART2_led(true);
      app_ppi_free_channel(0, UART_RX_PIN_NUMBER, TAG_TX_PIN_NUMBER);
      app_ppi_free_channel(1, TAG_RX_PIN_NUMBER, UART_TX_PIN_NUMBER);
      app_ppi_configure_channel(0, BV_RX_PIN_NUMBER, UART_TX_PIN_NUMBER);

      break;

  
    default:
        break;
    }
  }
  else {
    switch (uart_conf)
    { 
    case 0:
      app_hdw_set_UART1_led(false);
      app_hdw_set_UART2_led(false);

      err_code = app_uart_module_uninit();
      APP_ERROR_CHECK(err_code);

      err_code = app_uart_init_PB();
      APP_ERROR_CHECK(err_code);

    break;

    case 1:
      app_hdw_set_UART1_led(true);
      app_hdw_set_UART2_led(false);

      err_code = app_uart_module_uninit();
      APP_ERROR_CHECK(err_code);

      err_code = app_uart_init_TAG();
      APP_ERROR_CHECK(err_code);

    break;

    case 2:
      app_hdw_set_UART1_led(false);
      app_hdw_set_UART2_led(true);

      err_code = app_uart_module_uninit();
      APP_ERROR_CHECK(err_code);

      err_code = app_uart_init_BV();
      APP_ERROR_CHECK(err_code);

      break;

  
    default:
        break;
    }
  }

  NRF_LOG_INFO("UART_BLE : %d", uart_conf);


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
  mode++;
  if (mode > NB_MODE - 1)
  {
    mode = 0;
  }
  app_hdw_select_mode();
  nrf_delay_ms(300); //Si pas de debounce
}

void app_hdw_read_UART_BTN()
{
  uart_conf++;
  if (uart_conf > NB_UART_CONF - 1){
    uart_conf = 0;
  }
  app_hdw_select_UART();
  nrf_delay_ms(300); //Si pas de debounce

}

void app_hdw_read_V_BAT()
{
  nrf_saadc_value_t voltage;
  int32_t output_value = 0;
  uint8_t i;
  uint8_t count = 1;

  for (i = 0; i < count; i++) {
    app_saadc_get_channel(0, &voltage);
    output_value += voltage;
  }
  output_value /= count;

  //NRF_LOG_INFO("VOLTAGE RAW: %d", output_value);
  if (voltage < V_BAT_TRHL){
    app_hdw_set_low_bat_led(true);
  }
  else {
    app_hdw_set_low_bat_led(false);
  }
}

void app_hdw_detect_TAG()
{
  if (app_hdw_read_STCO() == STCO_OK){
    if (!(bool)nrf_gpio_pin_read(TAG_PWR_SENS)){
      app_hdw_set_TAG_pwr(true);
    }
      app_hdw_set_TAG_pwr(false);
  }
    //NRF_LOG_INFO("TAG Detection");

}

STARTCO_t app_hdw_read_STCO() {
  STARTCO_t STCO_state = STCO_ERROR;
  if ((bool)nrf_gpio_pin_read(STCO_OK_PIN)){
    STCO_state = STCO_OK;
  }
  else if ((bool)nrf_gpio_pin_read(STCO_OPEN_Z_PIN)) {
    STCO_state = STCO_OPEN_Z;
  }
  else if ((bool)nrf_gpio_pin_read(STCO_SHORT_Z_PIN)) {
    STCO_state = STCO_SHORT_Z;
  }
  return STCO_state;
}

void buttons_interrupt_init(void)
{
  if(!interrupt_initialized){

  // GPIO configuration for the INT1 pin.
  ret_code_t err_code;
  err_code = nrf_drv_gpiote_init();
  APP_ERROR_CHECK(err_code);


    UART_button_interrupt_init();
    mode_button_interrupt_init();
    interrupt_initialized = true;
    

  }
  nrf_drv_gpiote_in_event_enable(UART_SELECTOR_BTN, true);
  nrf_drv_gpiote_in_event_enable(MODE_SELECTOR_BTN, true);

  NRF_LOG_INFO("INTERRUPT_INIT");
}

void UART_button_interrupt_init(void) {

  nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
  in_config.pull = NRF_GPIO_PIN_PULLUP;

  nrf_drv_gpiote_in_init(UART_SELECTOR_BTN, &in_config, app_hdw_read_UART_BTN);

}

void mode_button_interrupt_init(void) {

  nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
  in_config.pull = NRF_GPIO_PIN_PULLUP;

  nrf_drv_gpiote_in_init(MODE_SELECTOR_BTN, &in_config, app_hdw_read_mode_BTN);
}

void app_hdw_set_uart_ble(bool enable) {
  uart_ble = enable;
  app_hdw_select_UART();
  NRF_LOG_INFO("UART_BLE_ENABLE : %d", enable);

}



