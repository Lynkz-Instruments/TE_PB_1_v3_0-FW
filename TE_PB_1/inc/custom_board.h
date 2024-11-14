/**
* @author Lucas Bonenfant (lucas@lynkz.ca)
* @brief 
* @version 1.0
* @date 2024-11-13
* 
* @copyright Copyright (c) Lynkz Instruments Inc, Amos 2024
* 
*/
#ifndef BOARD_CUSTOM_H
#define BOARD_CUSTOM_H
#ifdef __cplusplus
extern "C" {
#endif

//LEDS Default Pin Definition
#define INT_STCO_LED 27 // Internal STARTCO LED
#define INT_BV_LED 26 // Internal Bavard LED
#define MUX1_UART_LED 25 // 1st UART selector LED
#define MUX2_UART_LED 22 // 2st UART selector LED
#define LOW_BAT_LED 14 // 2st UART selector LED

//UART Default Pin Definition
#define UART_RX_PIN_NUMBER 23 // ProgBoard's ANNA UART Rx
#define UART_TX_PIN_NUMBER 24 // ProgBoard's ANNA UART Tx
#define TAG_RX_PIN_NUMBER 16 // TAG's ANNA UART Rx
#define TAG_TX_PIN_NUMBER 15 // TAG's ANNA UART Tx
#define BV_TX_PIN_NUMBER 13 // Bavard's UART Tx
#define SERIAL_RTS_PIN 0xFF
#define SERIAL_CTS_PIN 0xFF.

//Analog Switch Default Pin Definition
#define SW1 37 // Analog switch to connect the red terminal to the TAG's STCO input
#define SW2 36 // Analog switch to connect the red terminal to the Internal Bavard
#define SW3 31 // Analog switch to connect the Internal STCO to the Internal Bavard
#define SW4_5 30 // Analog switch to connect the black terminal to GND or the Internal STCO
#define SW6 29 // Analog switch to connect Bavard's filter circuit when the Internal Bavard is used

//STARTCO Default Pin Definition
#define STCO_OK 21 // HIGH if GND Check is passed
#define STCO_OPEN_Z 45 // HIGH if GND Check says OPEN CIRCUIT
#define STCO_SHORT_Z 38 // HIGH if GND Check says SHORT CIRCUIT

//Buttons Default Pin Definition
#define UART_SELECTOR_BTN 35 // Selector to chose which device communicate with UART through USB
#define MODE_SELECTOR_BTN 34 // Selector to change the configuration (INTERNAL/EXTERNAL STCO/Bavard)

//TAG Power Default Pin Definition
#define TAG_PWR 28 // Can power the TAG through this pin
#define TAG_PWR_SENS 20 // Can read if the tag is powered

//Battery sens Default Pin Definition
#define V_BAT_SENS 20 // Can read the battery's voltage


// Low frequency clock source to be used by the SoftDevice
#define NRF_CLOCK_LFCLKSRC {.source = NRF_CLOCK_LF_SRC_XTAL, \
.rc_ctiv = 0, \
.rc_temp_ctiv = 0, \
.xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}
#ifdef __cplusplus
}
#endif

#endif // BOARD_CUSTOM_H