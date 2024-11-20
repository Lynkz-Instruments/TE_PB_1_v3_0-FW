#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_ppi.h"
#include "app_error.h"
#include "app_ppi.h"

#define MAX_PPI_CHANNELS 3  // Par exemple, 3 canaux PPI

static nrf_ppi_channel_t ppi_channels[MAX_PPI_CHANNELS]; // Tableau pour les canaux PPI

void ppi_init(void)
{
    ret_code_t err_code;

    // Initialiser le module PPI
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    // Initialiser GPIOTE si ce n'est pas déjà fait
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }
}

void configure_ppi_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address)
{
    ret_code_t err_code;

    // Vérifier que l'indice est valide
    if (channel_index >= MAX_PPI_CHANNELS)
    {
        APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
    }

    // 2. Configurer GPIOTE pour la broche RX (événement sur changement d'état)
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true); // Sensibilité au changement d'état
    in_config.pull = NRF_GPIO_PIN_NOPULL;

    err_code = nrf_drv_gpiote_in_init(event_address, &in_config, NULL);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(event_address, true); // Activer les événements sur RX

    // 3. Configurer GPIOTE pour la broche TX (tâche de sortie)
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(true);
    err_code = nrf_drv_gpiote_out_init(task_address, &out_config);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_out_task_enable(task_address); // Activer les tâches sur TX

    // 5. Assigner PPI : relier l'événement RX à la tâche TX
    err_code = nrf_drv_ppi_channel_alloc(&ppi_channels[channel_index]);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(ppi_channels[channel_index],
                                          nrf_drv_gpiote_in_event_addr_get(event_address),
                                          nrf_drv_gpiote_out_task_addr_get(task_address));
    APP_ERROR_CHECK(err_code);
    enable_ppi_channel(channel_index);

}

void enable_ppi_channel(uint8_t channel_index)
{
    if (channel_index >= MAX_PPI_CHANNELS)
    {
        APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
    }

    // Activer le canal PPI
    ret_code_t err_code = nrf_drv_ppi_channel_enable(ppi_channels[channel_index]);
    APP_ERROR_CHECK(err_code);
}

void disable_ppi_channel(uint8_t channel_index)
{
    if (channel_index >= MAX_PPI_CHANNELS)
    {
        APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
    }

    // Désactiver le canal PPI
    ret_code_t err_code = nrf_drv_ppi_channel_disable(ppi_channels[channel_index]);
    APP_ERROR_CHECK(err_code);
}

void free_ppi_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address)
{
    if (channel_index >= MAX_PPI_CHANNELS)
    {
        APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
    }

    // Désactiver le canal PPI
    nrf_drv_ppi_channel_disable(ppi_channels[channel_index]);
    ret_code_t err_code = nrf_drv_ppi_channel_free(ppi_channels[channel_index]);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_disable(event_address);
    nrf_drv_gpiote_out_task_disable(task_address);

    nrf_drv_gpiote_in_uninit(event_address); // Activer les événements sur RX
    nrf_drv_gpiote_out_uninit(task_address); // Activer les tâches sur TX
    ppi_channels[channel_index] = 0;
}