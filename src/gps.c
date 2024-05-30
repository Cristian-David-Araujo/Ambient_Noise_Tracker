#include "gps.h"

void gps_init(gps_t *gGps, uart_inst_t *uart, uint8_t rx, uint8_t tx, uint16_t baudrate)
{
    // Initialize the GPS module
    gGps->uart = uart;
    gGps->rx = rx;
    gGps->tx = tx;
    gGps->baudrate = baudrate;
    gGps->status = false;
    gGps->data_available = false;
    
    // Initialize the UART
    uart_init(gGps->uart, gGps->baudrate);
    gpio_set_function(gGps->tx, GPIO_FUNC_UART);
    gpio_set_function(gGps->rx, GPIO_FUNC_UART);

    // Set the handler for the UART
    if (gGps->uart == uart0) {
        irq_set_exclusive_handler(UART0_IRQ, uart_read_handler);
        irq_set_enabled(UART0_IRQ, true);
    }
    else if (gGps->uart == uart1) {
        irq_set_exclusive_handler(UART1_IRQ, uart_read_handler);
        irq_set_enabled(UART1_IRQ, true);
    }

    // Set the UART IRQ enables for read data
    uart_set_irq_enables(gGps->uart, true, false);

}

void gps_send_command(gps_t *gGps, char *command)
{
    char checksum = command[1];
    char check_char[3] = {0};
    uint8_t i = 0;

    // Calculate the checksum
    for (i = 2; command[i] != '\0'; i++) {
        checksum ^= command[i];
    }

    // Convert checksum to ASCII
    check_char[0] = (checksum >> 4) < 10 ? (checksum >> 4) + '0' : (checksum >> 4) - 10 + 'A';
    check_char[1] = (checksum & 0x0F) < 10 ? (checksum & 0x0F) + '0' : (checksum & 0x0F) - 10 + 'A';

    // Send command and checksum
    uart_write(uart0, (const uint8_t *)command, strlen(command));
    uart_write(uart0, (const uint8_t *)"*", 1);
    uart_write(uart0, (const uint8_t *)check_char, 2);
    uart_write(uart0, (const uint8_t *)"\r\n", 2);

    // Wait for the response of the GPS module for 200ms min
}

void uart_read(uart_inst_t *uart, uint8_t *data, uint16_t len)
{

    if (uart_is_readable(uart)) {
        for (size_t i = 0; i < len; ++i) {
            *data++ = (uint8_t) uart_get_hw(uart)->dr;
        }
    }
    else {
        printf("UART is not readable\n");
    }
}

void uart_write(uart_inst_t *uart, uint8_t *data, uint16_t len)
{
    if (uart_is_writable(uart)) {
        for (size_t i = 0; i < len; ++i) {
            uart_putc(uart, *data++);
            //uart_get_hw(uart)->dr = *data++;
        }
    }
    else {
        printf("UART is not writable\n");
    }
}
