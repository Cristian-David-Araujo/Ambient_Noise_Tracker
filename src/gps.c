#include "gps.h"

//Constants for the GPS module
char const Temp[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static const double pi = 3.14159265358979324;
static const double a = 6378245.0;
static const double ee = 0.00669342162296594323;
static const double x_pi = 3.14159265358979324 * 3000.0 / 180.0;



void gps_init(gps_t *gps, uart_inst_t *uart, uint8_t rx, uint8_t tx, uint32_t baudrate)
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
    gps->buffer_index = 0;
    
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
    printf("GPS data: %s\n", gps->buffer);

    // Check if the GPS data is valid GNRMC or GPRMC
    if (strncmp(gps->buffer, "$GNRMC", 6) != 0 && strncmp(gps->buffer, "$GPRMC", 6) != 0) {
        printf("Invalid GPS data format.\n");
        return;
    }

    // Parse the GPS data
    char *token = strtok(gps->buffer, ",");
    uint16_t field_index = 0;

    while (token != NULL) {
        //printf("Token: %s\n", token);
        switch (field_index) {
            case 1:  // Time
                    // Extract hours
                    gps->time_h = (token[0] - '0') * 10 + (token[1] - '0');
                    // Extract minutes
                    gps->time_m = (token[2] - '0') * 10 + (token[3] - '0');
                    // Extract seconds
                    gps->time_s = (token[4] - '0') * 10 + (token[5] - '0');
                    // Extract milliseconds
                    gps->time_ms = (token[7] - '0') * 10 + (token[8] - '0');
                break;
            case 2:  // Status
                gps->status = (token[0] == 'A') ? 1 : 0; // 1: valid, 0: invalid
                break;
            case 3:  // Latitude
                if (strlen(token) >= 4) {
                    double lat_deg, lat_min;
                    sscanf(token, "%2lf%lf", &lat_deg, &lat_min);
                    gps->latitude = lat_deg + lat_min / 60.0;
                }
                break;
            case 4:  // Latitude Direction
                gps->latitude_area = token[0];
                if (gps->latitude_area == 'S') {
                    gps->latitude = -gps->latitude;
                }
                break;
            case 5:  // Longitude
                if (strlen(token) >= 5) {
                    double lon_deg, lon_min;
                    sscanf(token, "%3lf%lf", &lon_deg, &lon_min);
                    gps->longitude = lon_deg + lon_min / 60.0;
                }
                break;
            case 6:  // Longitude Direction
                gps->longitude_area = token[0];
                if (gps->longitude_area == 'W') {
                    gps->longitude = -gps->longitude;
                }
                break;
        }
        // Get the next token
        token = strtok(NULL, ",");
        field_index++;
    }
    
    printf("Time: %d:%d:%d\n", gps->time_h, gps->time_m, gps->time_s);
    printf("Status: %d\n", gps->status);
    printf("Latitude: %f %c\n", gps->latitude, gps->latitude_area);
    printf("Longitude: %f %c\n", gps->longitude, gps->longitude_area);
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
