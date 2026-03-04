
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "../include/SPI.h"

int spiFile = -1; 


int SPI_initPort(char* spiDevice)
{    

    // Open Device
    spiFile = open(spiDevice, O_RDWR);
    if (spiFile < 0) {
		perror("Error: Could not open SPI device");
        return -1;
    }

    // Set port parameters

	// SPI mode
    int spiMode = SPI_MODE_DEFAULT;
	int errorCheck = ioctl(spiFile, SPI_IOC_WR_MODE, &spiMode);
	if (errorCheck < 0) {
		perror("Error: Can't set SPI mode");
        close(spiFile);
        return -1;
    }

	// Set Max Speed (Hz)
    int speed = SPEED_HZ_DEFAULT;
	errorCheck = ioctl(spiFile, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (errorCheck < 0) {
		perror("Error: Set max speed hz failed");
        close(spiFile);
        return -1;
    }
    // Set Bits Per Word 
    uint8_t bits = SPI_BITS_PER_WORD_DEFAULT;
    if (ioctl(spiFile, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("Error: Can't set bits per word");
        close(spiFile);
        return -1;
    }

    return spiFile;
}

int SPI_transfer(int spiFile, uint8_t *sendBuf, uint8_t *receiveBuf, int length)
{
	struct spi_ioc_transfer transfer;
    memset(&transfer, 0, sizeof(transfer));
    
    transfer.tx_buf = (uintptr_t)sendBuf;
    transfer.rx_buf = (uintptr_t)receiveBuf;
    transfer.len = length;
    transfer.bits_per_word = SPI_BITS_PER_WORD_DEFAULT;
    transfer.speed_hz = SPEED_HZ_DEFAULT;     

    const int NUM_TRANSFERS = 1;
	int status = ioctl(spiFile, SPI_IOC_MESSAGE(NUM_TRANSFERS), &transfer);
	if (status < 0) {
		perror("Error: SPI Transfer failed");
    }

    return status;
}

