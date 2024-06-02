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
#include "hardware/pwm.h"

#include "functs.h"
#include "gpio_led.h"
#include "gpio_button.h"
#include "gps.h"
#include "microphone.h"
#include "liquid_crystal_i2c.h"

// I2C pins
#define PIN_SDA 14
#define PIN_SCL 15

//UART GPS Pins
#define GPS_TX 5
#define GPS_RX 4

led_rgb_t gLed;

flags_t gFlags; ///< Global variable that stores the flags of the interruptions pending
gps_t gGps; ///< Global variable the structure of the GPS
lcd_t gLcd; ///< Global variable the structure of the LCD

void initGlobalVariables(void)
{
    //Initialize the flags
    gFlags.W = 0;

    //Initialize the modules
    led_init(&gLed, 18);
    lcd_refresh_handler();

    gps_init(&gGps, uart1, GPS_TX, GPS_RX, 9600);

    lcd_init(&gLcd, 0x20, i2c1, 16, 2, 100, PIN_SDA, PIN_SCL);
    
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

void lcd_refresh_handler(void)
{
    // Set the alarm
    hw_clear_bits(&timer_hw->intr, 1u << TIMER_IRQ_1);

    gFlags.B.refresh_lcd = true;

    // Setting the IRQ handler
    irq_set_exclusive_handler(TIMER_IRQ_1, lcd_refresh_handler);
    irq_set_enabled(TIMER_IRQ_1, true);
    hw_set_bits(&timer_hw->inte, 1u << TIMER_IRQ_1); // Enable alarm0 for signal value calculation
    timer_hw->alarm[1] = (uint32_t)(time_us_64() + 1000000); // Set alarm0 to trigger in t_sample
}

void uart_read_handler(void)
{   
    char data = uart_getc(gGps.uart);
    //printf("%c", data);

    if (data == '$')
    {
        //Start of the data
        gGps.buffer_index = 0;
        gGps.data_available = true;
        //led_on(&gLed, 0x06); //Led on yellow
    }
    
    if ((data == '\r' || data == '\n' )&& gGps.data_available){
        //End of the data
        gGps.buffer[gGps.buffer_index] = '\0';
        gGps.data_available = false;
        gFlags.B.uart_read = 1;
        gGps.buffer_index = 0;

        //Disable the UART read interruption
        uart_set_irq_enables(gGps.uart, false, false);
        
    }
    else if (gGps.data_available && gGps.buffer_index < 255) {
        //Append the data to the buffer
        gGps.buffer[gGps.buffer_index] = data;
        gGps.buffer_index++;
    }
    else if (gGps.buffer_index >= 255){
        //Buffer overflow
        gGps.buffer_index = 0;
        gGps.data_available = false;
    }
}

void program(void)
{
    if (gFlags.B.uart_read){
        //Get the data from the GPS
        gps_get_GPGGA(&gGps);

        uart_clear_FIFO(gGps.uart);
        //enable the UART read interruption
        uart_set_irq_enables(gGps.uart, true, false);

        gps_check_data(&gGps);

        if(gGps.valid == 1){
            //GPS is working
            led_on(&gLed, 0x02); //Led on green
        }else{
            //GPS is not working
            led_on(&gLed, 0x04); //Led on red
        }

        //Clear the flag
        gFlags.B.uart_read = 0;  
    }

    if (gFlags.B.refresh_lcd){
         //Show the data on the LCD
        uint8_t str_0[16]; ///< Line 0 of the LCD

        //Clear the LCD
        //lcd_send_str_cursor(&gLcd, "                ", 0, 0);
        //lcd_send_str_cursor(&gLcd, "                ", 1, 0);
        sprintf((char *)str_0, "X: %f", gGps.latitude); 
        lcd_send_str_cursor(&gLcd, (char *)str_0, 0, 0); //Show the latitude
        sprintf((char *)str_0, "Y: %f", gGps.longitude);
        lcd_send_str_cursor(&gLcd, (char *)str_0, 1, 0); //Show the longitude
        sprintf((char *)str_0, "%d", gGps.fix_quality);
        lcd_send_str_cursor(&gLcd, (char *)str_0, 0, 15); //Show the fix quality
        sprintf((char *)str_0, "%d", gGps.num_satellites);
        lcd_send_str_cursor(&gLcd, (char *)str_0, 1, 15); //Show the number of satellites

        //Clear the flag
        gFlags.B.refresh_lcd = 0;
    }
}

bool check()
{
    if(gFlags.W){
        return true;
    }
    return false;
}
