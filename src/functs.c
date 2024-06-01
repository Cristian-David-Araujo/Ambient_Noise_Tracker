/**
 * \file        functs.c
 * \brief
 * \details
 * 
 * 
 * \author      MST_CDA
 * \version     0.0.1
 * \date        10/04/2024
 * \copyright   Unlicensed
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#include "functs.h"
#include "gpio_led.h"
#include "gpio_button.h"
#include "gps.h"
#include "microphone.h"


led_rgb_t gLed;

flags_t gFlags; ///< Global variable that stores the flags of the interruptions pending
gps_t gGps; ///< Global variable the structure of the GPS

void initGlobalVariables(void)
{
    //Initialize the flags
    gFlags.W = 0;

    //Initialize the modules
    led_init(&gLed, 18);
    gps_init(&gGps, uart1, 5, 4, 115200);
    
    led_on(&gLed, 0x04); //Led on red
}

void gpioCallback(uint num, uint32_t mask) 
{
    switch (mask)
    {
    case GPIO_IRQ_EDGE_RISE:
        
        break;

    case GPIO_IRQ_EDGE_FALL:

        break;
    
    default:
        printf("Happend what should not happens on GPIO CALLBACK\n");
        break;
    }
    
    gpio_acknowledge_irq(num, mask); ///< gpio IRQ acknowledge
}


void led_timer_handler(void)
{
    // Set the alarm
    hw_clear_bits(&timer_hw->intr, 1u << gLed.timer_irq);

    if (gLed.state){
        led_set_alarm(&gLed);
    }else {
        led_off(&gLed);
        irq_set_enabled(gLed.timer_irq, false); ///< Disable the led timer
    }
}

void uart_read_handler(void)
{   
    char data = uart_getc(gGps.uart);

    if (data == '$')
    {
        //Start of the data
        gGps.buffer_index = 0;
        gGps.data_available = true;
    }
    
    if (data == '\n' && gGps.data_available){
        //End of the data
        gGps.buffer[gGps.buffer_index] = '\0';
        gGps.data_available = false;
        gFlags.B.uart_read = 1;
        gGps.buffer_index = 0;

        //Disable the UART read interruption
        uart_set_irq_enables(gGps.uart, false, false);
        
    }
    else if (gGps.data_available) {
        //Append the data to the buffer
        gGps.buffer[gGps.buffer_index] = data;
        gGps.buffer_index++;
    }

    led_on(&gLed, 0x06); //Led on yellow
}

void program(void)
{
    if (gFlags.B.uart_read){
        //Get the data from the GPS
        gps_get_GNRMC(&gGps);
        led_on(&gLed, 0x02); //Led on green

        //enable the UART read interruption
        uart_set_irq_enables(gGps.uart, true, false);
        //Clear the flag
        gFlags.B.uart_read = 0;
    }
}

bool check()
{
    if(gFlags.W){
        return true;
    }
    return false;
}
