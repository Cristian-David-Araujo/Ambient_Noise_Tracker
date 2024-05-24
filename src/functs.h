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

#include "functs.h"
#include "gpio_led.h"
#include "gpio_button.h"
#include "gps.h"
#include "microphone.h"

/**
 * @brief This typedef is for generate a word on we have the flags of interrups pending.
 * @typedef flags_t
 */
typedef union{
    uint8_t W;
    struct{
        uint8_t key         :1; //keypad interruption pending
        uint8_t nfc         :1; //tag interruption pending
        uint8_t kpad_cols   :1; //keypad cols interruption pending
        uint8_t kpad_rows   :1; //keypad rows interruption pending
        uint8_t             :3;
    }B;
}flags_t;

/**
 * @brief This function initializes the global variables of the system: keypad, signal generator, button, and DAC.
 * 
 */
void initGlobalVariables(void);

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
 * @brief Configure the led Alarm
 * 
 * @param led 
 */
void led_set_alarm(led_rgb_t *led);

#endif // __FUNTCS_

