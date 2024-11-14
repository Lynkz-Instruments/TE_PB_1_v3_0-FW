/**
* @file app_error.c
* @author Emile Laplante (emile@lynkz.ca)
* @brief 
* @version 1.0
* @date 2022-05-11
* 
* @copyright Copyright (c) 2022
* 
*/
#include "app_error_lynkz.h"

/*lint -save -e14 */
/**
 * Overwrites the weak version in app_error_weak.c
 */
void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    __disable_irq();
    NRF_LOG_FINAL_FLUSH();

#ifndef DEBUG
    NRF_LOG_ERROR("Fatal error");
#else
    switch (id)
    {
#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT
        case NRF_FAULT_ID_SD_ASSERT:
            NRF_LOG_ERROR("SOFTDEVICE: ASSERTION FAILED");
            NRF_LOG_PROCESS();
            sd_softdevice_disable();
            NVIC_SystemReset();
            break;
        case NRF_FAULT_ID_APP_MEMACC:
            NRF_LOG_ERROR("SOFTDEVICE: INVALID MEMORY ACCESS");
            NRF_LOG_PROCESS();
            sd_softdevice_disable();
            NVIC_SystemReset();
            break;
#endif
        case NRF_FAULT_ID_SDK_ASSERT:
        {
            assert_info_t * p_info = (assert_info_t *)info;
            NRF_LOG_ERROR("ASSERTION FAILED at %s:%u",
                          p_info->p_file_name,
                          p_info->line_num);
            break;
        }
        case NRF_FAULT_ID_SDK_ERROR:
        {
            error_info_t * p_info = (error_info_t *)info;
            NRF_LOG_ERROR("ERROR %u [%s] at %s:%u\r\nPC at: 0x%08x",
                          p_info->err_code,
                          nrf_strerror_get(p_info->err_code),
                          p_info->p_file_name,
                          p_info->line_num,
                          pc);
             NRF_LOG_ERROR("End of error report");
            break;
        }
        default:
            NRF_LOG_ERROR("UNKNOWN FAULT at 0x%08X", pc);
            NRF_LOG_PROCESS();
            sd_softdevice_disable();
            NVIC_SystemReset();
            break;
    }
#endif

    NRF_BREAKPOINT_COND;
    // On assert, the system can only recover with a reset.

#ifndef DEBUG
    NRF_LOG_WARNING("System reset");
    NVIC_SystemReset();
#else
    app_error_save_and_stop(id, pc, info);
#endif // DEBUG
}
/*lint -restore */

/**
 * @}
 */