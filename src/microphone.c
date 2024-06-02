/**
 * \file        microphone.c
 * \brief       This proyect implements a microphone wich uses a ADC to get the audio signal and 
 *             a DMA to transfer the data to a buffer.
 * \details     

 * 
 * \author      MST_CDA
 * \version     0.0.1
 * \date        05/10/2023
 * \copyright   Unlicensed
 * 
 */

#include "microphone.h"

#include "functs.h"

void mphone_init(mphone_t *mphone, uint8_t gpio_num, uint32_t sample, uint8_t en_gpio)
{
    ///< Initialize the microphone structure
    mphone->gpio_num = gpio_num;
    mphone->en_gpio = en_gpio;
    mphone->spl_index = 0;
    mphone->en = false;
    mphone->dma_irq = 0;
    mphone->adc_chan = 26 - gpio_num; ///< channel 0 is GPIO 26, channel 1 is GPIO 27, etc.
    mphone->sample = sample;

    ///< Initialize the GPIO
    gpio_init(en_gpio);
    gpio_set_dir(en_gpio, GPIO_OUT);
    gpio_put(en_gpio, 1); ///< active low

    ///< Initialize the ADC
    adc_gpio_init(gpio_num);
    adc_init();
    adc_set_clkdiv(6.5*MHZ/sample); ///< Set the ADC clock to the sample rate
    adc_select_input(mphone->adc_chan); ///< Select the ADC channel
    adc_fifo_setup(
        true,   ///< Write each completed conversion to the sample FIFO
        true,   ///< Enable DMA data request (DREQ)
        1,      ///< DREQ is raised when at least 1 sample is present in the FIFO
        true,   ///< To see ERR bit in ADC result
        false   ///< Do not shift the result to 8 bits
    );

    ///< Initialize the DMA
    mphone->dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(mphone->dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_ADC);

    dma_channel_configure(
        mphone->dma_chan,   ///< Channel to configure 
        &c,
        &mphone->adc_buffer[0], ///< Write address
        &adc_hw->fifo,      ///< Read address
        MPHONE_SIZE_BUFFER, ///< Number of transfers
        false               ///< Don't start immediately
    );

    ///< Tell the DMA to raise IRQ line 0 when the channel finishes a block transfer
    dma_channel_set_irq0_enabled(mphone->dma_chan, true);

    ///< Enable the interrupt in the NVIC
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    ///< Print and load the SPL values
    mphone_load_print_spl_location(mphone);
}

void mphone_calculate_spl(mphone_t *mphone)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < MPHONE_SIZE_BUFFER; i++)
    {
        sum += mphone->adc_buffer[i];
    }
    double avg = sum/MPHONE_SIZE_BUFFER;
    mphone->spl[mphone->spl_index] = 20*pow(log10(sum/REF_PRESSURE), 2);
    mphone->lat[mphone->spl_index] = mphone->lat_v;
    mphone->lon[mphone->spl_index] = mphone->lon_v;
}

void mphone_store_spl_location(mphone_t *mphone)
{
    // An array of 256 bytes, multiple of FLASH_PAGE_SIZE. Database is 60 bytes.
    uint32_t buf[3*FLASH_PAGE_SIZE/sizeof(uint32_t)]; ///< The spl array sizes 200 bytes.

    // Copy the database into the buffer
    for (int i = 0; i < MPHONE_SIZE_SPL;){
        buf[i] = mphone->spl[i]; i++; ///< Casting? (uint32_t)
        buf[i] = mphone->lat[i]; i++;
        buf[i] = mphone->lon[i]; i++;
    }
    // Program buf[] into the first page of this sector
    // Each page is 256 bytes, and each sector is 4K bytes
    // Erase the last sector of the flash
    flash_safe_execute(mphone_wrapper, NULL, 500);

    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(FLASH_TARGET_OFFSET, (uint8_t *)buf, 3*FLASH_PAGE_SIZE);
    restore_interrupts (ints);
}

void mphone_load_print_spl_location(mphone_t *mphone)
{
    // Compute the memory-mapped address, remembering to include the offset for RAM
    uint32_t addr = XIP_BASE +  FLASH_TARGET_OFFSET;
    uint32_t *ptr = (uint32_t *)addr; ///< Place an int pointer at our memory-mapped address

    // Load the inventory from the flash memory
    printf("SPL, Latitude, Longitude\n");
    // for (int i = 0; i < 3*MPHONE_SIZE_SPL;){///< Casting? (uint32_t)
    //     mphone->spl[i] = ptr[i]; printf("%f, ", mphone->spl[i]); i++;
    //     mphone->lat[i] = ptr[i]; printf("%f, ", mphone->lat[i]); i++;
    //     mphone->lon[i] = ptr[i]; printf("%f\n", mphone->lon[i]); i++;
    // }
    printf("End\n");
}
