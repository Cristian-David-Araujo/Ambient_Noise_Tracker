#include "gps.h"

//Constants for the GPS module
char const Temp[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static const double pi = 3.14159265358979324;
static const double a = 6378245.0;
static const double ee = 0.00669342162296594323;
static const double x_pi = 3.14159265358979324 * 3000.0 / 180.0;



void gps_init(gps_t *gps, uart_inst_t *uart, uint8_t rx, uint8_t tx, uint16_t baudrate)
{
    // Initialize the GPS module
    gps->uart = uart;
    gps->rx = rx;
    gps->tx = tx;
    gps->baudrate = baudrate;
    gps->status = false;
    gps->data_available = false;

    //Initialize the GPS buffer with 0
    memset(gps->buffer, 0, sizeof(gps->buffer));
    
    // Initialize the UART
    uart_init(gps->uart, gps->baudrate);
    gpio_set_function(gps->tx, GPIO_FUNC_UART);
    gpio_set_function(gps->rx, GPIO_FUNC_UART);

    // Set the handler for the UART
    if (gps->uart == uart0) {
        irq_set_exclusive_handler(UART0_IRQ, uart_read_handler);
        irq_set_enabled(UART0_IRQ, true);
    }
    else if (gps->uart == uart1) {
        irq_set_exclusive_handler(UART1_IRQ, uart_read_handler);
        irq_set_enabled(UART1_IRQ, true);
    }

    // Set the UART IRQ enables for read data
    uart_set_irq_enables(gps->uart, true, false);

}

void gps_send_command(gps_t *gps, char *command)
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
    uart_write(uart0, (uint8_t *)command, strlen(command));
    uart_write(uart0, (uint8_t *)"*", 1);
    uart_write(uart0, (uint8_t *)check_char, 2);
    uart_write(uart0, (uint8_t *)"\r\n", 2);

    // Wait for the response of the GPS module for 200ms min
}

void gps_get_GNRMC(gps_t *gps)
{
    // Variables for the GPS data
    uint16_t add = 0, x = 0, z = 0, i = 0;
    uint32_t Time = 0;
    uint32_t latitude = 0, longitude = 0;

    // Initialize the GPS data
    gps->status = 0;
    gps->time_h = 0;
    gps->time_m = 0;
    gps->time_s = 0;

    // Read the GPS data
    uart_read(gps->uart, (uint8_t *)gps->buffer, GPS_BUFFSIZE);
    printf("%s\r\n", gps->buffer);

    add = 0;
    while (add < GPS_BUFFSIZE - 71) {
        // Check if the GPS data is valid GNRMC or GPRMC
        if (gps->buffer[add] == '$' && gps->buffer[add + 1] == 'G' &&
            (gps->buffer[add + 2] == 'N' || gps->buffer[add + 2] == 'P') &&
            gps->buffer[add + 3] == 'R' && gps->buffer[add + 4] == 'M' && gps->buffer[add + 5] == 'C') {

            x = 0;
            for (z = 0; x < 12; z++) {
                if (gps->buffer[add + z] == '\0') {
                    break;
                }
                if (gps->buffer[add + z] == ',') {
                    x++;
                    if (x == 1) { // The first comma is followed by time
                        Time = 0;
                        for (i = 0; gps->buffer[add + z + i + 1] != '.'; i++) {
                            if (gps->buffer[add + z + i + 1] == '\0') {
                                break;
                            }
                            if (gps->buffer[add + z + i + 1] == ',') {
                                break;
                            }
                            Time = (gps->buffer[add + z + i + 1] - '0') + Time * 10;
                        }

                        gps->time_h = Time / 10000 + 8;
                        gps->time_m = Time / 100 % 100;
                        gps->time_s = Time % 100;
                        if (gps->time_h >= 24) {
                            gps->time_h = gps->time_h - 24;
                        }
                    }
                    
                    else if (x == 2) {
                        // A indicates that it has been positioned
                        // V indicates that there is no positioning.
                        gps->status = (gps->buffer[add + z + 1] == 'A') ? 1 : 0;
                    }
                    
                    else if (x == 3) {
                        latitude = 0;
                        // Calculate latitude
                        for (i = 0; gps->buffer[add + z + i + 1] != ','; i++) {
                            if (gps->buffer[add + z + i + 1] == '\0') {
                                break;
                            }
                            if (gps->buffer[add + z + i + 1] == '.') {
                                continue;
                            }
                            latitude = (gps->buffer[add + z + i + 1] - '0') + latitude * 10;
                        }
                        gps->latitude = latitude / 1000000.0;
                    } 
                    
                    else if (x == 4) {
                        gps->latitude_area = gps->buffer[add + z + 1];
                    } 
                    
                    else if (x == 5) {
                        longitude = 0;
                        // Calculate longitude
                        for (i = 0; gps->buffer[add + z + i + 1] != ','; i++) {
                            if (gps->buffer[add + z + i + 1] == '\0') {
                                break;
                            }
                            if (gps->buffer[add + z + i + 1] == '.') {
                                continue;
                            }
                            longitude = (gps->buffer[add + z + i + 1] - '0') + longitude * 10;
                        }
                        gps->longitude = longitude / 1000000.0;
                    } 
                    
                    else if (x == 6) {
                        gps->longitude_area = gps->buffer[add + z + 1];
                    }
                }
            }
            add = 0;
            break;
        }

        // If the buffer is empty
        if (gps->buffer[add + 5] == '\0') {
            add = 0;
            break;
        }

        // If the buffer is full
        add++;
        if (add > GPS_BUFFSIZE) {
            add = 0;
            break;
        }
    }
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
