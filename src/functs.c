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

#define BUTTON_GPIO 10
#define LED_GPIO 0
#define MPHONE_GPIO 26

volatile enum{
    WAIT,       ///< The system is waiting for the GPS to hook (red led blinking)
    READY,      ///< GPS is hooked and the system is ready to measure (green led)
    MEASURE,    ///< The system is measuring the noise for 10s (yellow led)
    DONE,       ///< The system has finished the measurement and is sending the data (orange led)
    ERROR       ///< An anomaly has occurred (red led for 3s)
} gSystemState;

led_rgb_t gLed;         ///< Global variable that stores the led information
flags_t gFlags;         ///< Global variable that stores the flags of the interruptions pending
gpio_button_t gButton;  ///< Global variable that stores the button information
mphone_t gMphone;       ///< Global variable that stores the microphone information

void initGlobalVariables(void)
{
    led_init(&gLed, LED_GPIO, 1000000);
    gFlags.W = 0;
    button_init(&gButton, BUTTON_GPIO);
    mphone_init(&gMphone, MPHONE_GPIO, 44100); 
}

void initPWMasPIT(uint8_t slice, uint16_t milis, bool enable)
{
    assert(milis<=262);                  // PWM can manage interrupt periods greater than 262 milis
    float prescaler = (float)SYS_CLK_KHZ/500;
    assert(prescaler<256); // the integer part of the clock divider can be greater than 255 
                 // ||   counter frecuency    ||| Period in seconds taking into account de phase correct mode |||   
    uint32_t wrap = (1000*SYS_CLK_KHZ*milis/(prescaler*2*1000)); // 500000*milis/2000
    assert(wrap<((1UL<<17)-1));
    // Configuring the PWM
    pwm_config cfg =  pwm_get_default_config();
    pwm_config_set_phase_correct(&cfg, true);
    pwm_config_set_clkdiv(&cfg, prescaler);
    pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_FREE_RUNNING);
    pwm_config_set_wrap(&cfg, wrap);
    pwm_set_irq_enabled(slice, true);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    pwm_init(slice, &cfg, enable);
}

void program(void)
{
    if (gFlags.B.button){
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

void gpioCallback(uint num, uint32_t mask) 
{
    ///< Start measuring the noise when the button is pressed and the system is ready
    if (num == gButton.KEY.gpio_num && gSystemState == READY){
        pwm_set_enabled(0, true); ///< Enable the button debouncer
        button_set_irq_enabled(&gButton, false); ///< Disable the button IRQs
        button_set_zflag(&gButton); ///< Set the flag that indicates that a zero was detected on button
        gButton.KEY.dbnc = 1; ///< Set the flag that indicates that debouncer is active
    }
    ///< Stop measuring the noise when the button is pressed and the system is measuring. Red led will be on for 3s
    else if (num == gButton.KEY.gpio_num && gSystemState == MEASURE) {

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

void adc_handler(void)
{
}

void dma_handler(void)
{
}

void pwm_handler(void)
{
    bool button;
    switch (pwm_get_irq_status_mask())
    {
    case 0x01UL:
        button = gpio_get(gButton.KEY.gpio_num);
        if(button_is_2nd_zero(&gButton)){
            if(!button){
                button_set_irq_enabled(&gButton, true); // Enable the GPIO IRQs
                pwm_set_enabled(0, false);    // Disable the button debouncer
                signal_calculate(&gSignal); // Recalculate the signal values
                gButton.KEY.dbnc = 0;
            }
            else
                button_clr_zflag(&gButton);
        }
        else{
            if(!button)
                button_set_zflag(&gButton);
        }
        pwm_clear_irq(0); // Acknowledge slice 2 PWM IRQ
        break;
    
    default:
        break;
    }
}
