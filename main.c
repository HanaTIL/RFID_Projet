#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include "MFRC522.h"  
#include "SPI.h" 
#include <string.h>

//#define MODE_WRITE  // Uncomment this to build the WRITER
#define MODE_READ    // Uncomment this to build the READER




int main(void)
{
    MFRC522_statusCodes_t status; 
    byte defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // default key , in every new card

#ifdef MODE_WRITE
    byte nameBuffer[16] = "Hana Til       "; //Set your personal data
    byte jobBuffer[16] = "Embedded dev   ";
    byte sectorTrailer[16] = {
    0xFD, 0xCF, 0x1F, 0x20, 0x39, 0x85, // Key A (6 bytes)
    0xFF, 0x07, 0x80,                   // Access Bits (3 bytes - Factory Default)
    0x69,                               // User Byte (1 byte)
    0xFF, 0xF6, 0x45, 0x81, 0xAF, 0xEE  // Key B (6 bytes)  
    }; // use it when you want to write to sector trailor, keep in mind you can't access that specific sector with the default key anymore
    byte privateKeyA[6] = {0xFD, 0xCF, 0x1F, 0x20, 0x39, 0x85}; // Set your personal key, use this in case you changed the keyA 
    byte privateKeyB[6] = {0xFF, 0xF6, 0x45, 0x81, 0xAF, 0xEE};  // Set your personal key, use this in case you changed the keyB 

#endif

#ifdef MODE_READ
    byte testBuffer[18]; // we use it to read data
    byte testBuffer2[18]; // we use it only in case of reading two blocks
    byte privateKeyA[6] = {0xFD, 0xCF, 0x1F, 0x20, 0x39, 0x85}; // write your personal key, use this in case you changed the keyA in write function in sector trailor
    byte privateKeyB[6] = {0xFF, 0xF6, 0x45, 0x81, 0xAF, 0xEE};  // write your personal key, use this in case you changed the keyB in write functionin sector trailor
#endif



    //  Initialize The Linux SPI Driver
    spiFile = SPI_initPort("/dev/spidev0.0"); 
    if (spiFile < 0) {
        return 1; 
    }

    // Initialize the MFRC522 Chip (Registers & Antenna)
    // This uses the 'spiFile' handle to send the 0x80, 0xA9, 0x3D commands
    MFRC522_init();


    //  Verify communication (The "Handshake")
    // If this shows 0x00 or 0xFF, the wiring is wrong!
    MFRC522Version(); 

    printf("--- RFID Door Lock System Active ---\n");
    printf("Ready to scan. Press Ctrl+C to exit.\n");



    // The Infinite Scanning Loop
    for(;;)
    {
        // Is a card in the radio field?
        if(PICC_IsNewCardPresent())
        {
            PICC_ReadCardSerial();
            PICCDetails(&UID_t);
#ifdef MODE_WRITE
        //Write "Hana Til" to the very first available data block, change to personal data
        status =  MIFARE_Write(PICC_CMD_MF_AUTH_KEY_A, 0x01, nameBuffer);
        status = MIFARE_Write(PICC_CMD_MF_AUTH_KEY_A, 0x02, defaultKey, jobBuffer); 

        //status = MIFARE_Write(PICC_CMD_MF_AUTH_KEY_A, 0x13, defaultKey, sectorTrailer); uncomment in case you want to change the sector trailor
            
        if (status == MFRC522_STATUS_OK) 
        {
            printf("[DEBUG] Write Command Accepted by Card.\n");
        }
        else 
        {
            printf("[ERROR] Write failed! Card returned status: %d\n", status);
        } //printf for debugging

#endif

#ifdef MODE_READ
            //  we use this to read some data from the card 
            status = MIFARE_Read(PICC_CMD_MF_AUTH_KEY_A, 0x01, defaultKey, testBuffer);
            status = MIFARE_Read(0x60, 0x02, defaultKey, testBuffer2);

            //printf("status main: %d \n", status);
            if (status == 0) 
            { // MFRC522_STATUS_OK
            printf("Successfully Read Block 1!\n");
            printf("Successfully Read Block 2!\n");

            /*printf("Hex Data: ");
            for (int i = 0; i < 16; i++) {
            printf("%02X ", testBuffer[i]); // Print each byte in Hexadecimal
            }
            printf("\n");*/ //uncomment in case of reading the uid , data block 0

            printf("Name: ");
            for (int i = 0; i < 16; i++) 
            {
             // Only print if it's a readable character (A-Z, 0-9, etc.)
            if (testBuffer[i] >= 32 && testBuffer[i] <= 126) 
            {
                printf("%c", testBuffer[i]);
            } 
            else
            {
                printf("."); // Show non-printable bytes as dots
            }
            }
            printf("\n---\n");
            printf("Job: ");
            for (int i = 0; i < 16; i++) 
            {
             // Only print if it's a readable character (A-Z, 0-9, etc.)
            if (testBuffer2[i] >= 32 && testBuffer2[i] <= 126) 
            {
                printf("%c", testBuffer2[i]);
            } 
            else
            {
                printf("."); // Show non-printable bytes as dots
            }
            }
            printf("\n---\n");

            }
#endif         
           MIFARE_StopSession(); 
           printf("Card processed and session closed.\n");
       
		   }

        
        usleep(100000); 
    }

    //Cleanup (Close the file descriptor before exiting)
    close(spiFile);
    return 0;
}