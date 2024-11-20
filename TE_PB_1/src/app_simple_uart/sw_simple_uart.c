/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include <stdint.h>

#include "nrf.h"
#include "sw_simple_uart.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

static volatile uint8_t m_send_byte;
static uint8_t m_txd_pin_number;
static volatile bool m_uart_busy;
static simple_sw_baud_rate_t baud_rate;

#define UART_STOP_BIT_POS       8
#define UART_STOP_BIT_ACTIVE    1

/*
| START | d0 | d1 | d2 | d3 | d4 | d5 | d6 | d7 | STOP |
START = active low.
d0 = LSB, active high
...
d7 = MSB, active high
STOP = active low.

After STOP is issued, keep TXD high.
*/

void TIMER1_IRQHandler()
{
    if(NRF_TIMER1->EVENTS_COMPARE[0])
    {
        static uint8_t bit_pos;
        NRF_TIMER1->EVENTS_COMPARE[0] = 0;
        
        if (bit_pos < 8)
        {
            (m_send_byte & (1 << bit_pos)) ? nrf_gpio_pin_set(m_txd_pin_number) : nrf_gpio_pin_clear(m_txd_pin_number);
            bit_pos++;            
        }
        else if (bit_pos == UART_STOP_BIT_POS)
        {
            bit_pos++;
            nrf_gpio_pin_set(m_txd_pin_number);
        }
        // We have to wait until STOP bit is finished before releasing m_uart_busy.
        else if (bit_pos == 9)
        {            
            bit_pos = 0;
            NRF_TIMER1->TASKS_STOP = 1;            
            m_uart_busy = false;
        }
            
    }
}

static void sw_uart_timer_init(simple_sw_baud_rate_t baud_rate)
{
    NVIC_EnableIRQ(TIMER1_IRQn);
    NRF_TIMER1->PRESCALER = 0; // 16MHz resolution (CLK/2^prescaler)   
    NRF_TIMER1->INTENSET = TIMER_INTENSET_COMPARE0_Msk;
    NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
    NRF_TIMER1->CC[0] = (uint32_t)baud_rate;
}

bool simple_uart_put(uint8_t cr)
{
    if (!m_uart_busy)
    {
        m_uart_busy = true;
        m_send_byte = cr;    
         
        // We set start bit active (low)
        nrf_gpio_pin_clear(m_txd_pin_number);       
        NRF_TIMER1->TASKS_START = 1;
        return true;
    }
    return false;
}

void simple_uart_putstring(const uint8_t *str)
{
  uint_fast8_t i = 0;
  uint8_t ch = str[i++];
  while (ch != '\0')
  {
    while(simple_uart_put(ch) == false);
    ch = str[i++];
  }    
}

void simple_sw_uart_config(uint8_t txd_pin_number, simple_sw_baud_rate_t baud_rate)
{
    nrf_gpio_cfg_output(txd_pin_number);
    m_txd_pin_number = txd_pin_number;      
    sw_uart_timer_init(baud_rate);
}
