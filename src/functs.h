/**
 * \file        functs.h
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.1
 * \date        10/04/2024
 * \copyright   Unlicensed
 */

#ifndef __FUNTCS_
#define __FUNTCS_

#include <stdint.h>

/**
 * @brief This typedef is for generate a word on we have the flags of interrups pending.
 * @typedef flags_t
 */
typedef union{
    uint8_t W;
    struct{
        uint8_t button      :1; //Button interruption pending
        uint8_t mphone_dma  :1; //DMA interruption pending
        uint8_t             :3;
    }B;
}flags_t;

/**
 * @brief This function initializes the global variables of the system: keypad, signal generator, button, and DAC.
 * 
 */
void initGlobalVariables(void);

/**
 * @brief This function initializes a PWM signal as a periodic interrupt timer (PIT).
 * Each slice will generate interruptions at a period of milis miliseconds.
 * Due to each slice share clock counter (period), events with diferents periods 
 * must not be generated in the same slice, i.e, they must not share channel.
 * 
 * @param slice 
 * @param milis Period of the PWM interruption in miliseconds
 * @param enable 
 */
void initPWMasPIT(uint8_t slice, uint16_t milis, bool enable);

/**
 * @brief This function is the main, here the program is executed when a flag of interruption is pending.
 * 
 */
void program(void);

/**
 * @brief This function checks if there are a flag of interruption pending for execute the program.
 * 
 * @return true When there are a flag of interruption pending
 * @return false When there are not a flag of interruption pending
 */
bool check();

// -------------------------------------------------------------
// ---------------- Callback and handler functions -------------
// -------------------------------------------------------------

/**
 * @brief This function is the main callback function for the GPIO interruptions.
 * 
 * @param num 
 * @param mask 
 */
void gpioCallback(uint num, uint32_t mask);

/**
 * @brief Handler for the programmable timer
 * 

 */
void led_timer_handler(void);

/**
 * @brief Handler for the DMA interruption
 * 
 */
void dma_handler(void);

/**
 * @brief Handler for the PWM interruption
 * 
 */
void pwm_handler(void);



#endif // __FUNTCS_

