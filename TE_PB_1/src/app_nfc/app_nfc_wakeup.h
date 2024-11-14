#ifndef APP_NFC_WAKEUP_H
#define APP_NFC_WAKEUP_H

#define APP_NFC_WAKEUP_UICR_OFFSET 0
#define APP_NFC_WAKEUP_ALWAYS_ON 0

#include <sdk_errors.h>

/**
 * @brief Init the app nfc wake up module
 * @brief When the module in initiated, write something in the nfc tag to wake up the device
 * @brief Erase the nfc tag to put the device to sleep
 * @brief The sleep_function is used to put the device to sleep
 *
 * @return the data size
 */
ret_code_t app_nfc_wakeup_init(void (sleep_function)(void));

#endif //APP_NFC_WAKEUP_H
