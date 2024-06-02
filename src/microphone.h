/**
 * \file        microphone.h
 * \brief
 * \details
 * \author      MST_CDA
 * \version     0.0.1
 * \date        07/04/2024
 * \copyright   Unlicensed
 */
#ifndef __MICROPHONE_H__
#define __MICROPHONE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "pico/flash.h"

#define MPHONE_SIZE_BUFFER 5120 ///< Size of the ADC buffer.
#define MPHONE_SIZE_SPL 50 ///< Size of the Sound Pressure Level array.
#define MPHONE_SAMPLES_PER_PLACE 10 ///< Number of samples to calculate the SPL in one place.
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) ///< Flash-based address of the last sector
#define REF_PRESSURE 0.000020

/**
 * @typedef mphone_t 
 *
 * @brief Structure to manage a microphone connected to a ADC.
 * 
 */
typedef struct _mphone_t{
    uint8_t adc_chan;
    uint8_t dma_chan;
    uint8_t dma_irq;
    uint8_t adc_irq;
    uint32_t sample;
    uint8_t gpio_num;
    uint16_t adc_buffer[MPHONE_SIZE_BUFFER];
    double lat[MPHONE_SIZE_SPL]; ///< SPL array to store the Latitude values of each SPL value.
    double lon[MPHONE_SIZE_SPL]; ///< SPL array to store the Longitude values of each SPL value.
    double spl[MPHONE_SIZE_SPL];
    double lat_v;
    double lon_v;
    uint8_t spl_index; ///< Index of the SPL array. It is going to count up to MPHONE_SIZE_SPL.
    uint32_t dma_time; ///< Time which takes the DMA to transfer the data.
    bool dma_done; ///< Flag to indicate that the DMA has finished the transfer.
    bool en;
}mphone_t;

/**
 * @brief Initialize the microphone.
 * 
 * @param mphone 
 * @param gpio_num must be between 26 and 29.
 * @param sample sample rate in Hz for the ADC.
 */
void mphone_init(mphone_t *mphone, uint8_t gpio_num, uint32_t sample);

/**
 * @brief Trigger the DMA to start the data transfer.
 * 
 * @param mphone 
 */
static inline void mphone_dma_trigger(mphone_t *mphone)
{
    adc_fifo_drain(); ///< Clear the FIFO
    adc_run(true); ///< Start the ADC to free running mode
    dma_channel_set_read_addr(mphone->dma_chan, &adc_hw->fifo, true); ///< Start the DMA
}

/**
 * @brief Calculate the Sound Pressure Level.
 * From the ADC buffer, calculate one sigle point of SPL, and store it in the SPL array.
 * As said in the ISO 1683-1:1998, the SPL is calculated as: Lp = 20 * log10(Pa/Pref) dB, 
 * where Pa is the ponderated pressure, and Pref is the reference pressure, 20uPa.
 * 
 * @param mphone 
 * @param lat Latitude of the place where the SPL was measured.
 * @param lon Longitude of the place where the SPL was measured.
 */
void mphone_calculate_spl(mphone_t *mphone);

/**
 * @brief Store the SPL array in non-volatile memory.
 * 
 * @param mphone 
 */
void mphone_store_spl_location(mphone_t *mphone);

/**
 * @brief Load and print all the information (SPL, Latitude, Longitud) from non-volatile memory.
 * 
 * @param mphone 
 */
void mphone_load_print_spl_location(mphone_t *mphone);

/**
 * @brief Definition of the wrapper function for the flash_safe_execute function,
 * which is used to erase the last sector of the flash memory.
 * 
 */
static inline void mphone_wrapper()
{
    flash_range_erase((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
}

#endif // __MICROPHONE_H__