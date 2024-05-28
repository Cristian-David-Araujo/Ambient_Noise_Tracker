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

void mphone_init(mphone_t *mphone, uint8_t gpio_num, uint32_t sample)
{
    ///< Initialize the microphone structure
    mphone->gpio_num = gpio_num;
    mphone->spl_index = 0;
    mphone->en = false;
    mphone->dma_irq = 0;
    mphone->adc_chan = 26 - gpio_num; ///< channel 0 is GPIO 26, channel 1 is GPIO 27, etc.
    mphone->sample = sample;

    ///< Initialize the ADC
    adc_gpio_init(26 + mphone->adc_chan);
    adc_init();
    adc_set_clkdiv(USB_CLK_KHZ*1000/sample); ///< Set the ADC clock to the sample rate
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

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block transfer
    dma_channel_set_irq0_enabled(mphone->dma_chan, true);

    // Enable the interrupt in the NVIC
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

}

void mphone_calculate_spl(mphone_t *mphone)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < MPHONE_SIZE_BUFFER; i++)
    {
        sum += mphone->adc_buffer[i];
    }
    sum = sum/MPHONE_SIZE_BUFFER;
    mphone->spl[mphone->spl_index] = 20*pow(log10(sum/0.000020), 2);
}

void mphone_store_spl(mphone_t *mphone)
{
}
