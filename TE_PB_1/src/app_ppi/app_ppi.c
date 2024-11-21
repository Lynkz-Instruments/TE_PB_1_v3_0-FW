/**
* @author Lucas Bonenfant (lucas@lynkz.ca)
* @brief 
* @version 1.0
* @date 2024-11-21
* 
* @copyright Copyright (c) Lynkz Instruments Inc, Amos 2024
* 
*/

#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_ppi.h"
#include "app_error.h"
#include "app_ppi.h"

#define MAX_PPI_CHANNELS 3  // Maximum of 3 channels

static nrf_ppi_channel_t ppi_channels[MAX_PPI_CHANNELS]; // Array of channels

void app_ppi_init(void)
{
    ret_code_t err_code;

    // Initializing PPI Module
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    // Initializing GPIOTE
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }
}

void app_ppi_configure_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address)
{
    ret_code_t err_code;

    // Index number verification
    if (channel_index >= MAX_PPI_CHANNELS)
    {
        APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
    }

    // 2. Configure GPIOTE for RX pin (event on changing state)
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true); // Sense changing state and initial state is HIGH
    in_config.pull = NRF_GPIO_PIN_NOPULL;

    err_code = nrf_drv_gpiote_in_init(event_address, &in_config, NULL);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(event_address, true); // Enable event on RX pin

    // 3. Configure GPIOTE for TX pin (output TASK)
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(true);
    err_code = nrf_drv_gpiote_out_init(task_address, &out_config);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_out_task_enable(task_address); // Enable TX task

    // 5. Assign PPI : link RX event to TX task
    err_code = nrf_drv_ppi_channel_alloc(&ppi_channels[channel_index]);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(ppi_channels[channel_index],
                                          nrf_drv_gpiote_in_event_addr_get(event_address),
                                          nrf_drv_gpiote_out_task_addr_get(task_address));
    APP_ERROR_CHECK(err_code);

    // Enable the channel
    app_ppi_enable_channel(channel_index);

}

void app_ppi_enable_channel(uint8_t channel_index)
{
    // Index number verification
    if (channel_index >= MAX_PPI_CHANNELS)
    {
        APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
    }

    // Enable PPI channel
    ret_code_t err_code = nrf_drv_ppi_channel_enable(ppi_channels[channel_index]);
    APP_ERROR_CHECK(err_code);
}

void app_ppi_erase_channel(uint8_t channel_index)
{
    // Index number verification
    if (channel_index >= MAX_PPI_CHANNELS)
    {
        APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
    }

    // Disable PPI channel
    nrf_drv_ppi_channel_disable(ppi_channels[channel_index]);

    // Free PPI channel
    ret_code_t err_code = nrf_drv_ppi_channel_free(ppi_channels[channel_index]);
    APP_ERROR_CHECK(err_code);

}

void app_ppi_free_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address)
{
    // Index number verification
    if (channel_index >= MAX_PPI_CHANNELS)
    {
        APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
    }
    
    // Erase PPI channel
    app_ppi_erase_channel(channel_index);

    // Free PPI event pin
    nrf_drv_gpiote_in_event_disable(event_address);
    nrf_drv_gpiote_in_uninit(event_address);

    // Free PPI task pin
    nrf_drv_gpiote_out_task_disable(task_address);
    nrf_drv_gpiote_out_uninit(task_address);

    ppi_channels[channel_index] = 0;
}