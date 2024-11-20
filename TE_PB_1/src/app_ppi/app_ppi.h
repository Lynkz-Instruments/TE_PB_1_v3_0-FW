#ifndef APP_PPI
#define APP_PPI

void ppi_init();

void configure_ppi_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address);

void enable_ppi_channel(uint8_t channel_index);

void disable_ppi_channel(uint8_t channel_index);
void free_ppi_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address);


#endif