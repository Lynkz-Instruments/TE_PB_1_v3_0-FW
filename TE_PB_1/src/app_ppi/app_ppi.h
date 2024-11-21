#ifndef APP_PPI
#define APP_PPI

void app_ppi_init();

void app_ppi_configure_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address);

void app_ppi_enable_channel(uint8_t channel_index);

void app_ppi_disable_channel(uint8_t channel_index);

void app_ppi_free_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address);


#endif