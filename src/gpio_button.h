/**
 * \file        gpio_button.h
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.1
 * \date        07/04/2024
 * \copyright   Unlicensed
 */

#ifndef __GPIO_BUTTON_
#define __GPIO_BUTTON_

#include <stdint.h>
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

#include "functs.h"

/**
 * @typedef gpio_button_t 
 *
 * @brief Structure to manage a single button connected to a GPIO.
 * 
 */
typedef struct{
    struct {
        uint8_t gpio_num    : 5;        // GPIO gpio_num number
        uint8_t dzero       : 1;        // Flag for double zero
        uint8_t nkey        : 1;        // Flag that indicates that a key was pressed
        uint8_t dbnc        : 1;        // Flag that indicates that debouncer is active
    }KEY;
    uint8_t slice;                      // Slice used for the debouncer
}gpio_button_t;

/**
 * @brief 
 * 
 * @param button 
 * @param gpio_num 
 */
static inline void button_init(gpio_button_t *button, uint8_t gpio_num)
{
    button->KEY.dzero = 0;
    button->KEY.nkey = 0;
    button->KEY.dbnc = 0;
    button->KEY.gpio_num = gpio_num;
    button->slice = 0;

    gpio_init(button->KEY.gpio_num);
    gpio_set_dir(button->KEY.gpio_num, GPIO_IN);
    gpio_pull_down(button->KEY.gpio_num);

    gpio_set_irq_enabled_with_callback(button->KEY.gpio_num, GPIO_IRQ_EDGE_RISE, true, gpioCallback);
}

/**
 * @brief Enable/disable the interruptions for the button.
 * 
 * @param button 
 * @param enable true to enable the button interruption, false to disable.
 */
static inline void button_set_irq_enabled(gpio_button_t *button, bool enable)
{
    gpio_set_irq_enabled(button->KEY.gpio_num, GPIO_IRQ_EDGE_RISE, enable);
}

/** 
 * \brief This method sets to one the flag that indicates that a zero was detected on button
 * \param button   Pointer to keypad data structure
 */
static inline void button_set_zflag(gpio_button_t *button)
{
    button->KEY.dzero = 1;
}
/** 
 * \brief This method clears the flag that indicates that a zero was detected on button
 * \param button   Pointer to keypad data structure
 */
static inline void button_clr_zflag(gpio_button_t *button)
{
    button->KEY.dzero = 0;
}

/** 
 * \brief This method returns true if a first zero was detected on button
 * \param button   Pointer to keypad data structure
 */
static inline bool button_is_2nd_zero(gpio_button_t *button)
{
    return button->KEY.dzero;
}

static inline void button_setup_pwm_dbnc(gpio_button_t *button)
{
    pwm_set_enabled(button->slice, true); ///< Enable the button debouncer
    button_set_irq_enabled(button, false); ///< Disable the button IRQs
    button_set_zflag(button); ///< Set the flag that indicates that a zero was detected on button
    button->KEY.dbnc = 1;       ///< Set the flag that indicates that debouncer is active
}

#endif // __GPIO_BUTTON_