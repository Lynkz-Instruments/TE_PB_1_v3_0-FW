/**
* @author Lucas Bonenfant (lucas@lynkz.ca)
* @brief 
* @version 1.0
* @date 2024-11-21
* 
* @copyright Copyright (c) Lynkz Instruments Inc, Amos 2024
* 
*/

#ifndef APP_PPI
#define APP_PPI

/**
 * @brief Function to enable PPI and GPIOTE.
 * 
 */
void app_ppi_init();

/**
 * @brief Function to create a channel and configure its pins.
 * 
 */
void app_ppi_configure_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address);

/**
 * @brief Function to activate a channel.
 * 
 */
void app_ppi_enable_channel(uint8_t channel_index);

/**
 * @brief Function to deactivate a channel.
 * 
 */
void app_ppi_erase_channel(uint8_t channel_index);

/**
 * @brief Function to erase a channel et free its associated pins.
 * 
 */
void app_ppi_free_channel(uint8_t channel_index, uint32_t event_address, uint32_t task_address);


void app_ppi_configure_channel_RX(uint8_t channel_index, uint32_t event_address, uint32_t task_address);

#endif