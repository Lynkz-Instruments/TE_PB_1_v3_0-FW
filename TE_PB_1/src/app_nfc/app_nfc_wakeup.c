#include "app_nfc_wakeup.h"
#include "app_nfc.h"
#include "nrf_delay.h"
#include "scheduler.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define APP_NFC_WAKE_UP_COMMAND_SIZE 14

// For now, there's only one command that we can send to the device by NFC. 
// 1> PWR_ON
enum APP_NFC_WAKEUP_COMMAND_ENUM 
{
  APP_NFC_WAKEUP_COMMAND_PWR_ON,
  APP_NFC_WAKEUP_COMMAND_UNKNOWN
};

volatile bool app_nfc_wakeup_dev_on = false;
const uint8_t nfc_pwr_on_command[APP_NFC_WAKE_UP_COMMAND_SIZE]  = {0xd1,0x01,0x09,0x54,0x02,0x65,0x6e,0x50,0x57,0x52,0x5f,0x4f,0x4e,0x00};
const uint8_t nfc_pwr_off_command[APP_NFC_WAKE_UP_COMMAND_SIZE] = {0xd1,0x01,0x0a,0x54,0x02,0x65,0x6e,0x50,0x57,0x52,0x5f,0x4f,0x46,0x46};

// Function pointer of deep_sleep function.
void (*app_nfc_wakeup_sleep_function)(void) = NULL;

// Function to handle NFC tag updates.
void app_nfc_wakeup_tag_update_handle(const uint8_t *p_data, size_t data_length);

// Function to evalutate the command received by NFC.
static uint8_t app_nfc_wakeup_evaluate_command(const uint8_t* const command_buffer, uint16_t command_size);

// Function to load device state UICR.
static void app_nfc_wakeup_load_uicr(void);

// Function to save the device state UICR.
static void app_nfc_wakeup_save_uicr(void);

// Function to deep_sleep when device state id off.
static void app_nfc_wakeup_sleep(void);

// Function to rewrite the command received to the tag.
static void app_nfc_wakeup_rewrite_nfc(void);

ret_code_t app_nfc_wakeup_init(void (sleep_function)(void))
{
  // Save the sleep function
  app_nfc_wakeup_sleep_function = sleep_function;

  // Load uicr config
  app_nfc_wakeup_load_uicr();

  // Initialize the nfc module
  app_nfc_init();

  // Put the nfc module in t4t mode
  ret_code_t err_code_t4t = app_nfc_t4t_mode();
  if(err_code_t4t != NRF_SUCCESS) return err_code_t4t;

  app_nfc_t4t_set_handler(app_nfc_wakeup_tag_update_handle);
  app_nfc_wakeup_rewrite_nfc();

  // Check if device is awake
  if( app_nfc_wakeup_dev_on){
    return NRF_SUCCESS;
  }

  // Wait for user command
  for(int i = 0; i < 300; i++){
    nrf_delay_ms(100);
    if(!app_nfc_get_and_clear_event_flag()){ 
      break; 
    }
  }
  
  // Device need to sleep.
  if( !app_nfc_wakeup_dev_on ){
    app_nfc_wakeup_sleep();
  }

  return NRF_SUCCESS;
}

void app_nfc_wakeup_tag_update_handle(const uint8_t *p_data, size_t data_length)
{
  uint8_t buffer[APP_NFC_T4T_BUFFER_LEN - 2];
  uint32_t length = app_nfc_t4t_get_data(buffer, APP_NFC_T4T_BUFFER_LEN - 2);

  const uint8_t command_id = app_nfc_wakeup_evaluate_command(buffer, length); // Command evaluation.

  switch(command_id){
    case APP_NFC_WAKEUP_COMMAND_PWR_ON:
      //NRF_LOG_INFO("PWR_ON request");
      app_nfc_wakeup_dev_on = true; // Device state change to ON.
      SCH_Add_Task(app_nfc_wakeup_save_uicr, 0, 0, true); // Add this function to the scheduler to change the UICR.
      break;

    case APP_NFC_WAKEUP_COMMAND_UNKNOWN:
      break;
  }

  app_nfc_wakeup_rewrite_nfc(); // Rewrite the received command in NFC tag.
}

static uint8_t app_nfc_wakeup_evaluate_command(const uint8_t* const command_buffer, uint16_t command_size)
{
  // The received command size is too long.
  if(command_size > 14){
    return APP_NFC_WAKEUP_COMMAND_UNKNOWN;
  }
  
  // Checking the command size to validate if this match one of the commands.
  bool pwr_on = false;
  if(command_size == 13){
    pwr_on = true;
  }
  
  // Checking each characters received to validate the command.
  for(int i = 0 ; i < command_size; i++){
    const uint8_t value = command_buffer[i];
    if(value != nfc_pwr_on_command[i]){ 
      pwr_on = false;
    }
  }
  
  // Return the command type.
  if(pwr_on){
    return  APP_NFC_WAKEUP_COMMAND_PWR_ON;
  }
  
  // Command not recognized.
  return APP_NFC_WAKEUP_COMMAND_UNKNOWN;
}

static void app_nfc_wakeup_load_uicr(void)
{
  const uint32_t  uicr_config = app_uicr_get(APP_NFC_WAKEUP_UICR_OFFSET);
  app_nfc_wakeup_dev_on = uicr_config == 1;
}

static void app_nfc_wakeup_save_uicr(void)
{
  uint32_t  uicr_config = 0;
  bool need_reset = false;

  if(app_nfc_wakeup_dev_on) uicr_config = 1;
  
  // Storing the UICR state. This step is important to store the device state.
  // This device reads this UICR at boot time and init NFC if needed.
  app_uicr_set(APP_NFC_WAKEUP_UICR_OFFSET, uicr_config);
  
  // This reset is important to make sure this UICR value is taken into account.
  NRF_LOG_INFO("The device will reset.");
  nrf_delay_ms(50);
  NVIC_SystemReset();
}

static void app_nfc_wakeup_sleep(void)
{
  #if (APP_NFC_WAKEUP_ALWAYS_ON != 1)
  APP_ERROR_CHECK(app_nfc_wake_up_mode()); // Setup NFC mode.
  app_nfc_wakeup_sleep_function(); // Device sleep.
  #endif
}

static void app_nfc_wakeup_rewrite_nfc(void)
{
  if(app_nfc_wakeup_dev_on){
    NRF_LOG_INFO("TAG rewrite PWR_ON");
    app_nfc_t4t_set_data(nfc_pwr_on_command, 13);
  }
  else{
    NRF_LOG_INFO("TAG rewrite PWR_OFF");
    app_nfc_t4t_set_data(nfc_pwr_off_command, 14);
  }
}
