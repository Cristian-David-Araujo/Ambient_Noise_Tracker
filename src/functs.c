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

void initGlobalVariables(void)
{
    led_init(&gLed, 0);
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


void led_set_alarm(led_rgb_t *led)
{
    // Interrupt acknowledge
    hw_clear_bits(&timer_hw->intr, 1u << led->timer_irq);
    led->state = false;

    // Setting the IRQ handler
    irq_set_exclusive_handler(led->timer_irq, led_timer_handler);
    irq_set_enabled(led->timer_irq, true);
    hw_set_bits(&timer_hw->inte, 1u << led->timer_irq); ///< Enable alarm1 for keypad debouncer
    timer_hw->alarm[led->timer_irq] = (uint32_t)(time_us_64() + led->time); ///< Set alarm1 to trigger in 100ms
}



void program(void)
{
    if (gFlags.B.key){
        gFlags.B.key = 0;
        printf("Key interruption\n");
    }
}

bool check()
{
    if(gFlags.W){
        return true;
    }
    return false;
}
