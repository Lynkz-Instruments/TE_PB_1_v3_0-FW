#include "app_nfc.h"
#include "nrf5_utils.h"

volatile bool flag_nfc_field_on = false;
volatile bool event_flag = false;
volatile uint32_t t4t_data_length = 0;

uint8_t t4t_buffer[APP_NFC_T4T_BUFFER_LEN];

////////////////////////// NFC wake up callback ////////////////////////////////////////////////////////
void nrft_callback( nrfx_nfct_evt_t const *p_event )
{
  event_flag = true;
  if( p_event->evt_id == NRFX_NFCT_EVT_FIELD_DETECTED){
    flag_nfc_field_on = true;
  }

  if( p_event->evt_id == NRFX_NFCT_EVT_FIELD_LOST){
    flag_nfc_field_on = false;
  }
}

//////////////////////////// NFC T4T callback ////////////////////////////////////////////////////////////////
void(*app_nfc_t4t_handler)(const uint8_t *p_data, size_t data_length) = NULL;

void t4t_callback(void *p_context, nfc_t4t_event_t event, const uint8_t *p_data, size_t data_length, uint32_t flags)
{
  event_flag = true;
  switch (event){
    case NFC_T4T_EVENT_FIELD_ON:
      flag_nfc_field_on = true;
      break;
    case NFC_T4T_EVENT_FIELD_OFF:
      flag_nfc_field_on = false;
      break;
    case NFC_T4T_EVENT_NDEF_READ:
      break;
    case NFC_T4T_EVENT_NDEF_UPDATED:
      if(data_length > 0){
        t4t_data_length = data_length;
        if(app_nfc_t4t_handler != NULL) app_nfc_t4t_handler(p_data, data_length);
      }
      break;
    default:
      break;
  }
}

void app_nfc_t4t_set_handler( void(handler)(const uint8_t *p_data, size_t data_length) )
{
  app_nfc_t4t_handler = handler;
}

/////////////////////////////// NFC init //////////////////////////////////////////////////////////////////////
uint8_t app_nfc_mode = 0;

void app_nfc_init(void)
{
  app_nfc_mode = 0;
  t4t_data_length = 0;
}

///////////////////////////// NFC wake up mode ////////////////////////////////////////////////////////
ret_code_t app_nfc_wake_up_mode(void)
{
  if(app_nfc_mode == 1){ return NRF_SUCCESS; }

  if(app_nfc_mode == 2){
    nfc_t4t_emulation_stop();
    nrfx_nfct_disable();
    nrfx_nfct_uninit();
  }

  const nrfx_nfct_config_t nrft_config = {
    .rxtx_int_mask = NRFX_NFCT_EVT_FIELD_DETECTED | NRFX_NFCT_EVT_FIELD_LOST,
    .cb = nrft_callback
  };

  if(nrfx_nfct_init(&nrft_config) != NRFX_SUCCESS){ return NRF_ERROR_INVALID_STATE; }

  flag_nfc_field_on = false;
  nrfx_nfct_enable();
  nrfx_nfct_state_force(NRFX_NFCT_STATE_SENSING);

  app_nfc_mode = 1;
  return NRF_SUCCESS;
}

//////////////////////////////////////// NFC T4T mode ///////////////////////////////////////////////////
ret_code_t app_nfc_t4t_mode(void)
{
  if(app_nfc_mode == 2){ return NRF_SUCCESS; }

  if(app_nfc_mode == 1){
    nrfx_nfct_disable();
    nrfx_nfct_uninit();
  }

  const ret_code_t err_code_setup = nfc_t4t_setup(t4t_callback, NULL);
  if(err_code_setup != NRF_SUCCESS){ return err_code_setup; }

  memset(t4t_buffer, 0, APP_NFC_T4T_BUFFER_LEN);
  t4t_data_length = 0;

  const ret_code_t return_value_payload_set = nfc_t4t_ndef_rwpayload_set(t4t_buffer,APP_NFC_T4T_BUFFER_LEN);
  if(return_value_payload_set != NRF_SUCCESS){ return return_value_payload_set; }

  const ret_code_t return_value_emulation_start = nfc_t4t_emulation_start();
  if(return_value_payload_set != NRF_SUCCESS){ return return_value_emulation_start; }

  app_nfc_mode = 2;
  return NRF_SUCCESS;

}

///////////////////////////////////////// NFC T4T get data ///////////////////////////////////////////////////
uint16_t app_nfc_t4t_get_data(uint8_t* buffer_p, uint16_t max_size)
{
  if( t4t_data_length <= max_size){
    memcpy(buffer_p, t4t_buffer+NLEN_FIELD_SIZE, t4t_data_length);
    return t4t_data_length;
  }
  else{
    memcpy(buffer_p, t4t_buffer+NLEN_FIELD_SIZE, max_size);
    return max_size;
  }
}

//////////////////////////////////////////// NFC T4T set data //////////////////////////////////////////////////
void app_nfc_t4t_set_data(const uint8_t* const buffer_p, uint16_t size)
{
  if( size <= (APP_NFC_T4T_BUFFER_LEN - NLEN_FIELD_SIZE)){
    memcpy(t4t_buffer+NLEN_FIELD_SIZE, buffer_p, size);
    t4t_buffer[0] = size >> 8;
    t4t_buffer[1] = size & 0x00ff;
    t4t_data_length = size;
  }
  else{
    const uint16_t buffer_size = APP_NFC_T4T_BUFFER_LEN - NLEN_FIELD_SIZE;
    memcpy(t4t_buffer+NLEN_FIELD_SIZE, buffer_p, buffer_size);
    t4t_buffer[0] = buffer_size >> 8;
    t4t_buffer[1] = buffer_size & 0x00ff;
    t4t_data_length = buffer_size;
  }
}

////////////////////////////////////// NFC t4t get data size //////////////////////////////////////////////
uint16_t app_nfc_t4t_get_size(void)
{
  return t4t_data_length;
}

//////////////////////////////////// NFC field detected ////////////////////////
bool app_nfc_get_and_clear_event_flag(void)
{
  const bool buffer = event_flag;
  event_flag = false;
  return buffer;
}