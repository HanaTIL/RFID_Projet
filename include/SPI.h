#ifndef SPI_SAMPLE_H
#define SPI_SAMPLE_H

/**
 * SPI Module
 * Provides an API for using either of the raspberry's two SPI devices.
 * Allows for easy port initialization and transfers.
 */

#include <stdint.h>

#define NUM_SPI_BUSES 1
#define SPI_MODE_DEFAULT 0
#define SPEED_HZ_DEFAULT 1000000
#define SPI_BITS_PER_WORD_DEFAULT 8
#define SPI_DEV_BUS0_CS0 "/dev/spidev0.0"
#define SPI_DEV_BUS0_CS1 "/dev/spidev0.1"
extern int spiFile; 




// Configures pins associated with the specified port number for SPI
// Opens the file associated with that port and the specified chipSelect number
// Returns the file descriptor
int SPI_initPort(char* spiDevice);

// Returns the status
// Status >= 0 is success, < 0 is error
int SPI_transfer(int spiFile, uint8_t *send, uint8_t *recv, int numBytes);

#endif