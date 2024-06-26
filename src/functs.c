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
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"

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
#define GPS_TX 4
#define GPS_RX 5

#define SYSTEM_CLK_HZ 48*MHZ
#define SYSTEM_CLK_KHZ 6500
#define ADC_SAMPLE_RATE_HZ MPHONE_SIZE_BUFFER/10

#define LCD_EN_GPIO 12
#define MPHONE_EN_GPIO 13
#define GPS_EN_GPIO 11
#define BUTTON_GPIO 2
#define DORMANT_GPIO 3
#define LED_GPIO 18
#define MPHONE_GPIO 26

system_t gSystem;  ///< Global variable that stores the state of the system
led_rgb_t gLed;         ///< Global variable that stores the led information
flags_t gFlags;         ///< Global variable that stores the flags of the interruptions pending
gpio_button_t gButton;  ///< Global variable that stores the button information
mphone_t gMphone;       ///< Global variable that stores the microphone information
gps_t gGps; ///< Global variable the structure of the GPS
lcd_t gLcd; ///< Global variable the structure of the LCD

void initGlobalVariables(void)
{
    //Initialize the flags
    gFlags.W = 0;

    //Initialize the modules
    led_init(&gLed, 18, 1000000); //Led on green
    gps_init(&gGps, uart1, GPS_TX, GPS_RX, 9600, GPS_EN_GPIO);
    lcd_init(&gLcd, 0x20, i2c1, 16, 2, 100, PIN_SDA, PIN_SCL, LCD_EN_GPIO);
    led_init(&gLed, LED_GPIO, 1000000);
    button_init(&gButton, BUTTON_GPIO);
    mphone_init(&gMphone, MPHONE_GPIO, ADC_SAMPLE_RATE_HZ, MPHONE_EN_GPIO);
  
    ///< Set the system state to DORMANT
    gSystem.state = DORMANT; 
    gSystem.usb = true;
    // clock_config();
    gpio_set_dormant_irq_enabled(BUTTON_GPIO, GPIO_IRQ_EDGE_RISE, true);
}

void initPWMasPIT(uint8_t slice, uint16_t milis, bool enable)
{
    assert(milis<=262);                  // PWM can manage interrupt periods greater than 262 milis
    float prescaler = (float)SYSTEM_CLK_KHZ/500;
    assert(prescaler<256); // the integer part of the clock divider can be greater than 255 
                 // ||   counter frecuency    ||| Period in seconds taking into account de phase correct mode |||   
    uint32_t wrap = (1000*SYSTEM_CLK_KHZ*milis/(prescaler*2*1000)); // 500000*milis/2000
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

void clock_config(void)
{
    ///< Reference Clock configuration
    clock_configure(clk_ref, 
                    CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH, // CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH
                    0, ///< No aux mux
                    SYSTEM_CLK_HZ,
                    SYSTEM_CLK_HZ);

    ///< Sys Clock configuration
    clock_configure(clk_sys, 
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, // CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF
                    0, ///< Using glitchless mux
                    SYSTEM_CLK_HZ,
                    SYSTEM_CLK_HZ);

    ///< CLK RTC = 0MHz
    clock_stop(clk_rtc);

    ///< ADC Clock configuration
    clock_configure(clk_adc, 
                    0,
                    CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH,
                    SYSTEM_CLK_HZ,
                    SYSTEM_CLK_HZ);

    ///< Peri Clock configuration
    clock_configure(clk_peri, 
                    0,                                          // CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,  // CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH
                    SYSTEM_CLK_HZ,
                    SYSTEM_CLK_HZ);

    pll_deinit(pll_sys);

    ///< USB Clock configuration
    if (!gSystem.usb){
        // clock_configure(clk_usb, 
        //                 0,                                          // CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLK_SYS
        //                 CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH,  // CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH
        //                 SYSTEM_CLK_HZ,
        //                 SYSTEM_CLK_HZ);

        // CLK USB = 0MHz
        clock_stop(clk_usb);
        pll_deinit(pll_usb); 

        // Can disable xosc
        xosc_disable();
                        
        // pll_deinit(pll_usb);
        // pll_init(pll_usb, 1, 1200 * KHZ * KHZ, 7, 7); ///< 24.549MHz
    }else {
        ///< Print the frequencies of the clocks
        measure_freqs();
    }
    
    ///< Reconfigure uart with new clocks
    setup_default_uart();
    stdio_init_all();
    if (frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB) <= 1) {
        led_setup_purple(&gLed); ///< Purple led
    }
}

void program(void)
{
    if (gFlags.B.wait){ ///< The system is waiting for the GPS to be hooked
        gFlags.B.wait = 0;
        printf_usb("WAIT \n");
        gLed.time = 1000000;    ///< 1s
        gLed.state = 1;         ///< Led on
        led_setup_red(&gLed);   ///< Red led
        mphone_configure_dma(&gMphone); ///< Configure the DMA for the microphone
        lcd_refresh_handler();
    }
    if (gFlags.B.meas){ ///< Start the measurement
        gFlags.B.meas = 0;
        printf_usb("MEASURE \n");
        gLed.time = 10000000;       ///< 10s
        led_setup_yellow(&gLed);    ///< Yellow led
        gMphone.dma_time = time_us_32(); ///< Start the DMA transfer
        mphone_dma_trigger(&gMphone);   ///< Start the DMA for the microphone
    }
    if (gFlags.B.error){ ///< An anomaly has occurred
        gFlags.B.error = 0;
        printf_usb("ERROR \n");
        gLed.time = 3000000;    ///< 3s
        led_setup_red(&gLed);   ///< Red led
    }
    if (gFlags.B.mphone_dma){ ///< The DMA has finished and the microphone has a new SPL
        gFlags.B.mphone_dma = 0;
        printf_usb("Microphone interruption\n");
        mphone_calculate_spl(&gMphone); ///< Calculate the Sound Pressure Level
        gSystem.state = DONE;       ///< The system has finished the measurement
        gLed.time = 2000000;        ///< 2s
        led_setup_orange(&gLed);    ///< Orange led
        mphone_store_spl_location(&gMphone); ///< Store the SPL array in non-volatile memory
    }
    if (gFlags.B.uart_read){
        //Get the data from the GPS
        gps_get_GPGGA(&gGps);

        uart_clear_FIFO(gGps.uart);
        //enable the UART read interruption
        uart_set_irq_enables(gGps.uart, true, false);

        gps_check_data(&gGps);

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

void gpioCallback(uint num, uint32_t mask) 
{
    if (num == gButton.KEY.gpio_num) {
        switch (gSystem.state)
        {
        case DORMANT: ///< Start the system when the button is pressed and the system is dormant. Like a power on button
            gSystem.state = WAIT;   ///< The system is waiting for the GPS to be hooked.
            button_setup_pwm_dbnc(&gButton); ///< Debounce setup
            lcd_enable(&gLcd);
            mphone_enable(&gMphone);
            gps_enable(&gGps);
            irq_set_enabled(gLed.timer_irq, true); ///< Enable the led timer
            irq_set_enabled(TIMER_IRQ_1, true); ///< Enable the lcd refresh timer
            lcd_initialization_timer_handler();
            break;

        case READY: ///< Start measuring the noise when the button is pressed and the system is ready
            if (gGps.valid){
                gSystem.state = MEASURE; ///< The system is measuring the noise
                gMphone.lat_v = gGps.latitude; ///< Store the latitude of the place where the SPL was measured
                gMphone.lon_v = gGps.longitude; ///< Store the longitude of the place where the SPL was measured
                button_setup_pwm_dbnc(&gButton); ///< Debounce setup
            }
            else {
                gSystem.state = ERROR; ///< The system is waiting for the GPS to be hooked
                gFlags.B.error = 1; ///< The system is waiting for the GPS to be hooked
            }
            break;

        case MEASURE: ///< Stop measuring the noise when the button is pressed and the system is measuring. 
            gSystem.state = ERROR; ///< An anomaly has occurred
            button_setup_pwm_dbnc(&gButton); ///< Debounce setup
            break;

        default: ///< The system is not ready to start the measurement
            // printf_usb("Button pressed but the system is not ready\n");
            break;
        }
    }
    gpio_acknowledge_irq(num, mask); ///< gpio IRQ acknowledge
}

void led_timer_handler(void)
{
    // Aknowledge the interrupt
    hw_clear_bits(&timer_hw->intr, 1u << gLed.timer_irq);

    if (gSystem.state == WAIT){
        if (gLed.state){
            led_setup_red(&gLed); ///< Red led
            gLed.state = 0;
        }else {
            led_on(&gLed, 0x00);
            led_set_alarm(&gLed);
            gLed.state = 1;
        }
        if (gGps.valid == 1) {
            gSystem.state = READY; ///< The system is ready to measure
            led_setup_green(&gLed); ///< Green led
        }
    }
    else if (gSystem.state == DONE || gSystem.state == ERROR){
        led_off(&gLed);
        lcd_disable(&gLcd);
        mphone_disable(&gMphone);
        gps_disable(&gGps);
        irq_set_enabled(gLed.timer_irq, false); ///< Disable the led timer
        irq_set_enabled(TIMER_IRQ_1, false); ///< Disable the lcd refresh timer
        gSystem.state = DORMANT; ///< The system is going to DORMANT state
    }

    if (gSystem.state == MEASURE) {
        if (gMphone.dma_done) { ///< The DMA transfer is done correctly
            gFlags.B.mphone_dma = 1;
            gMphone.dma_done = false;
        }
        else {
            gFlags.B.error = 1; ///< An anomaly has occurred: the DMA transfer is not done after 10s
            gSystem.state = ERROR; ///< An anomaly has occurred: the DMA transfer is not done after 10s
            printf_usb("ERROR: DMA transfer not done\n");
        }
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

void dma_handler(void)
{
    dma_irqn_acknowledge_channel(gMphone.dma_irq, gMphone.dma_chan); ///< Acknowledge the DMA IRQ
    gMphone.dma_done = true; ///< Set the flag that indicates that the DMA has finished
    gMphone.dma_time = time_us_32() - gMphone.dma_time; ///< Calculate the time which takes the DMA to transfer the data 
    uint8_t str[20];
    sprintf((char *)str, "DMA time: %d us\n", gMphone.dma_time);
    printf_usb((char *)str);
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
                gButton.KEY.dbnc = 0;
                switch (gSystem.state)
                {
                case WAIT:
                    gFlags.B.wait = 1; ///< The systen just power on and is waiting for the GPS to hook
                    break;
                case MEASURE:
                    gFlags.B.meas = 1; ///< Start the measurement
                    break;
                case ERROR:
                    gFlags.B.error = 1; ///< An anomaly has occurred
                    break;
                default:
                    break;
                }
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

    uint8_t str[30];
    sprintf((char *)str, "pll_sys = %dkHz\n", f_pll_sys);
    printf_usb((char *)str);

    sprintf((char *)str, "pll_usb = %dkHz\n", f_pll_usb);
    printf_usb((char *)str);

    sprintf((char *)str, "rosc = %dkHz\n", f_rosc);
    printf_usb((char *)str);

    sprintf((char *)str, "clk_sys = %dkHz\n", f_clk_sys);
    printf_usb((char *)str);

    sprintf((char *)str, "clk_peri = %dkHz\n", f_clk_peri);
    printf_usb((char *)str);

    sprintf((char *)str, "clk_usb = %dkHz\n", f_clk_usb);
    printf_usb((char *)str);

    sprintf((char *)str, "clk_adc = %dkHz\n", f_clk_adc);
    printf_usb((char *)str);

    sprintf((char *)str, "clk_rtc = %dkHz\n", f_clk_rtc);
    printf_usb((char *)str);
    // Can't measure clk_ref / xosc as it is the ref
}

void printf_usb(char *str)
{
    if (gSystem.usb)
        printf(str);
}
