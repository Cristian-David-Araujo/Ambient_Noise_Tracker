/**
 * \file        main.c
 * \brief       This proyect implements a ambient noise tracker.
 * \details     

 * 
 * \author      MST_CDA
 * \version     0.0.1
 * \date        05/10/2023
 * \copyright   Unlicensed
 * 
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/rosc.h"
#include "hardware/xosc.h"

#include "functs.h"

extern system_t gSystem;
extern flags_t gFlags;

int main() {
    stdio_init_all();
    sleep_ms(3000);
    printf("Run Program\n");

    // Initialize global variables
    initGlobalVariables();

    // PWM configuration
    initPWMasPIT(0, 100, false);     // 100ms for the button debounce
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_handler);

    while(1){
        while(gFlags.W){
            program();
        }
        if (gSystem.state == DORMANT){
            rosc_set_dormant(); // Set the system to dormant mode
        }
        else 
            __wfi(); // Wait for interrupt (Will put the processor into deep sleep until woken by the RTC interrupt)
        
        
    }
}

