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
#include "pico/sleep.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"

#include "functs.h"
#include "gpio_led.h"
#include "gpio_button.h"
#include "gps.h"
#include "microphone.h"

#define BUTTON_GPIO 10
#define LED_GPIO 0
#define MPHONE_GPIO 26

system_t gSystem;  ///< Global variable that stores the state of the system
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

    ///< Set the system state to DORMANT
    gSystem.state = DORMANT; 
    clock_config();
    gpio_set_dormant_irq_enabled(BUTTON_GPIO, GPIO_IRQ_EDGE_RISE, true);
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
        gFlags.B.button = 0;
        printf("Key interruption\n");
        gSystem.state = MEASURE;         ///< The system is measuring the noise
        mphone_dma_trigger(&gMphone);   ///< Start the DMA for the microphone
    }
    if (gFlags.B.mphone_dma){
        gFlags.B.mphone_dma = 0;
        printf("Microphone interruption\n");
        mphone_calculate_spl(&gMphone); ///< Calculate the Sound Pressure Level
        gMphone.spl_index++;
        if (gMphone.spl_index == MPHONE_SIZE_SPL) {
            gSystem.state = DONE;            ///< The system has finished the measurement
            gLed.time = 2000000;            ///< 2s
            led_setup_orange(&gLed);       ///< Orange led
            gMphone.spl_index = 0;
            mphone_store_spl(&gMphone);    ///< Store the SPL array in non-volatile memory
        }
    }
}

bool check()
{
    if(gFlags.W){
        return true;
    }
    return false;
}

void clock_config(void)
{
    ///< Print the frequencies of the clocks
    measure_freqs();
    ///< Reference Clock configuration
    uint src_hz = 6.5*MHZ;
    clock_configure(clk_ref, 
                    CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH, // CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH
                    0, ///< No aux mux
                    src_hz,
                    src_hz);

    ///< Sys Clock configuration
    clock_configure(clk_sys, 
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, // CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF
                    0, ///< Using glitchless mux
                    src_hz,
                    src_hz);
    
    ///< CLK USB = 0MHz
    clock_stop(clk_usb);

    ///< CLK RTC = 0MHz
    clock_stop(clk_rtc);

    ///< ADC Clock configuration
    clock_configure(clk_adc, 
                    0,
                    CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH,
                    src_hz,
                    src_hz);

    ///< Peri Clock configuration
    clock_configure(clk_peri, 
                    0,                                          // CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH,  // CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH
                    src_hz,
                    src_hz);

    pll_deinit(pll_sys);
    pll_deinit(pll_usb);

    ///< Can disable xosc
    xosc_disable();

    ///< Reconfigure uart with new clocks
    setup_default_uart();

    ///< Print the frequencies of the clocks
    measure_freqs();
}

void gpioCallback(uint num, uint32_t mask) 
{
    if (num == gLed.gpio_num && gSystem.state == WAIT){
        gLed.state = !gLed.state;
        led_setup_red(&gLed);
    }
    ///< Start measuring the noise when the button is pressed and the system is ready
    if (num == gButton.KEY.gpio_num && gSystem.state == READY){
        pwm_set_enabled(0, true); ///< Enable the button debouncer
        button_set_irq_enabled(&gButton, false); ///< Disable the button IRQs
        button_set_zflag(&gButton); ///< Set the flag that indicates that a zero was detected on button
        gButton.KEY.dbnc = 1; ///< Set the flag that indicates that debouncer is active
    }
    ///< Stop measuring the noise when the button is pressed and the system is measuring. Red led will be on for 3s
    else if (num == gButton.KEY.gpio_num && gSystem.state == MEASURE) {
        gSystem.state = ERROR; ///< An anomaly has occurred
        gLed.time = 3000000; ///< 3s
        led_setup_green(&gLed);
    }
    
    gpio_acknowledge_irq(num, mask); ///< gpio IRQ acknowledge
}


void led_timer_handler(void)
{
    // Set the alarm
    hw_clear_bits(&timer_hw->intr, 1u << gLed.timer_irq);

    if (gSystem.state == WAIT){
        gLed.state = !gLed.state;
        led_setup_red(&gLed);
    }
    else if (gSystem.state == READY || gSystem.state == MEASURE || gSystem.state == DONE || gSystem.state == ERROR) {
        led_off(&gLed);
        irq_set_enabled(gLed.timer_irq, false); ///< Disable the led timer
    }

}

void dma_handler(void)
{
    dma_irqn_acknowledge_channel(gMphone.dma_irq, gMphone.dma_chan); ///< Acknowledge the DMA IRQ
    gFlags.B.mphone_dma = 1; ///< Set the flag that indicates that the DMA has finished
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
                button_set_irq_enabled(&gButton, true); ///< Enable the GPIO IRQs
                pwm_set_enabled(0, false);      ///< Disable the button debouncer
                gFlags.B.button = 1;
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

void measure_freqs(void)
{
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys = %dkHz\n", f_pll_sys);
    printf("pll_usb = %dkHz\n", f_pll_usb);
    printf("rosc = %dkHz\n", f_rosc);
    printf("clk_sys = %dkHz\n", f_clk_sys);
    printf("clk_peri = %dkHz\n", f_clk_peri);
    printf("clk_usb = %dkHz\n", f_clk_usb);
    printf("clk_adc = %dkHz\n", f_clk_adc);
    printf("clk_rtc = %dkHz\n", f_clk_rtc);
    // Can't measure clk_ref / xosc as it is the ref
}
