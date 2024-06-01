/**
 * \file        gps.h
 * \brief       This module for use the GPS module L76X of WaveShare with the protocol NMEA 0183
 * \details
 * 
 * 
 * \author      CDA_MST
 * \version     0.0.1
 * \date        29/05/2024
 * \copyright   Unlicensed
 */

#ifndef __GPS_H__
#define __GPS_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#include "functs.h"

//Startup mode
#define GPS_HOT_START       "$PMTK101"
#define GPS_WARM_START      "$PMTK102"
#define GPS_COLD_START      "$PMTK103"
#define GPS_FULL_COLD_START "$PMTK104"

//Standby mode -- Exit requires high level trigger
#define GPS_SET_PERPETUAL_STANDBY_MODE      "$PMTK161"

#define GPS_SET_PERIODIC_MODE               "$PMTK225"
#define GPS_SET_NORMAL_MODE                 "$PMTK225,0"
#define GPS_SET_PERIODIC_BACKUP_MODE        "$PMTK225,1,1000,2000"
#define GPS_SET_PERIODIC_STANDBY_MODE       "$PMTK225,2,1000,2000"
#define GPS_SET_PERPETUAL_BACKUP_MODE       "$PMTK225,4"
#define GPS_SET_ALWAYSLOCATE_STANDBY_MODE   "$PMTK225,8"
#define GPS_SET_ALWAYSLOCATE_BACKUP_MODE    "$PMTK225,9"

//Set the message interval,100ms~10000ms
#define GPS_SET_POS_FIX         "$PMTK220"
#define GPS_SET_POS_FIX_100MS   "$PMTK220,100"
#define GPS_SET_POS_FIX_200MS   "$PMTK220,200"
#define GPS_SET_POS_FIX_400MS   "$PMTK220,400"
#define GPS_SET_POS_FIX_800MS   "$PMTK220,800"
#define GPS_SET_POS_FIX_1S      "$PMTK220,1000"
#define GPS_SET_POS_FIX_2S      "$PMTK220,2000"
#define GPS_SET_POS_FIX_4S      "$PMTK220,4000"
#define GPS_SET_POS_FIX_8S      "$PMTK220,8000"
#define GPS_SET_POS_FIX_10S     "$PMTK220,10000"

//Switching time output
#define GPS_SET_SYNC_PPS_NMEA_OFF   "$PMTK255,0"
#define GPS_SET_SYNC_PPS_NMEA_ON    "$PMTK255,1"

//Baud rate
#define GPS_SET_NMEA_BAUDRATE           "$PMTK251"
#define GPS_SET_NMEA_BAUDRATE_115200    "$PMTK251,115200"
#define GPS_SET_NMEA_BAUDRATE_57600     "$PMTK251,57600"
#define GPS_SET_NMEA_BAUDRATE_38400     "$PMTK251,38400"
#define GPS_SET_NMEA_BAUDRATE_19200     "$PMTK251,19200"
#define GPS_SET_NMEA_BAUDRATE_14400     "$PMTK251,14400"
#define GPS_SET_NMEA_BAUDRATE_9600      "$PMTK251,9600"
#define GPS_SET_NMEA_BAUDRATE_4800      "$PMTK251,4800"

//To restore the system default setting
#define GPS_SET_DEFAULT_SETTING               "$PMTK314,-1"

//Set NMEA sentence output frequencies 
#define GPS_SET_NMEA_OUTPUT "$PMTK314,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,0"

// Macros for NMEA 0183 sentences
#define GPS_NMEA_GGA "$GPGGA"  // Time, position, fix type data
#define GPS_NMEA_GLL "$GPGLL"  // Latitude, longitude, UTC time of position fix and status
#define GPS_NMEA_GSA "$GPGSA"  // GPS receiver operating mode, satellites used in the position solution, DOP values
#define GPS_NMEA_GSV "$GPGSV"  // Number of satellites in view, satellite ID numbers, elevation, azimuth, SNR values
#define GPS_NMEA_RMC "$GPRMC"  // Time, date, position, course, speed data
#define GPS_NMEA_VTG "$GPVTG"  // Course, speed information relative to the ground

// Macros for GPS NMEA buffer
#define GPS_BUFFSIZE 800


/**
 * @brief Struct for GPS module for module L76X of WaveShare with the protocol NMEA 0183
 * 
 */
typedef struct
{
    uart_inst_t *uart; //< UART instance (uart0 or uart1)
    uint8_t rx;  //< UART Pin number RX
    uint8_t tx;  //< UART Pin number TX
    uint32_t baudrate;  //< Baudrate of the GPS module
    bool status;  //< Avalibility GPS positions (1: Success, 0: Fail)
    bool data_available;  //< Data available in the UART

    // GPS data
    char buffer[GPS_BUFFSIZE];  //< Buffer for the GPS data
    uint16_t buffer_index;  //< Index of the buffer

    uint8_t time_h;  //< Time hour
    uint8_t time_m;  //< Time minutes
    uint8_t time_s;  //< Time seconds
    uint8_t time_ms;  //< Time milliseconds

    double latitude;  //< Latitude
    double longitude;  //< Longitude
    uint8_t longitude_area; //< Longitude area (E or W)
    uint8_t latitude_area;  //< Latitude area (N or S) 
}gps_t;


extern gps_t gGps;

/**
 * @brief This function initializes the GPS module L76X of WaveShare with the protocol NMEA 0183
 * 
 */
void gps_init(gps_t *gps, uart_inst_t *uart, uint8_t rx, uint8_t tx, uint32_t baudrate);

/**
 * @brief Send a command to the GPS, automatically calculate the checksum
 * 
 * @param gps GPS structure with the configuration
 * @param command Command to send to the GPS. The command string it must end with '\0' to avoid errors.
 */
void gps_send_command(gps_t *gps, char *command);

/**
 * @brief Get the data from the GPS module and store it in the buffer (GNRMC sentence)
 * 
 * @param gps GPS structure with the configuration
 */
void gps_get_GNRMC(gps_t *gps);

/*! \brief  Read from the UART witout blocking
 *  \ingroup hardware_uart
 *
 * This function blocks until len characters have been read from the UART
 *
 * \param uart UART instance. \ref uart0 or \ref uart1
 * \param data Buffer to accept received bytes
 * \param len The number of bytes to receive.
 */
void uart_read(uart_inst_t *uart, uint8_t *data, uint16_t len);

/*! \brief  Write to the UART witout blocking
 *  \ingroup hardware_uart
 *
 * This function blocks until len characters have been written to the UART
 *
 * \param uart UART instance. \ref uart0 or \ref uart1
 * \param data Buffer to send
 * \param len The number of bytes to send.
 */
void uart_write(uart_inst_t *uart, uint8_t *data, uint16_t len);

#endif // __GPS_H__