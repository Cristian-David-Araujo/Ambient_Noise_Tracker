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
#include "hardware/i2c.h"

#include "functs.h"


int main() {
    stdio_init_all();
    sleep_ms(5000);
    printf("Run Program\n");

    // Initialize global variables
    initGlobalVariables();


    while(1){
        while(check()){
            program();
        }
        __wfi(); // Wait for interrupt (Will put the processor into deep sleep until woken by the RTC interrupt)
    }
}

