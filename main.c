#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include "MFRC522.h"  
#include "SPI.h" 
#include <string.h>




int main(void)
{
    //  Initialize The Linux SPI Driver
    spiFile = SPI_initPort("/dev/spidev0.0"); 
    if (spiFile < 0) {
        return 1; 
    }
    MFRC522_statusCodes_t status; 

    // Initialize the MFRC522 Chip (Registers & Antenna)
    // This uses the 'spiFile' handle to send the 0x80, 0xA9, 0x3D commands
    MFRC522_init();


    


    //  Verify communication (The "Handshake")
    // If this shows 0x00 or 0xFF, the wiring is wrong!
    MFRC522Version(); 

    printf("--- RFID Door Lock System Active ---\n");
    printf("Ready to scan. Press Ctrl+C to exit.\n");

    // Member variable to hold the UID data
    //Uid uid_value;
    byte nameBuffer[16] = "Hana Til        ";
    byte testBuffer[18];

    // The Infinite Scanning Loop
    for(;;)
    {
        // Is a card in the radio field?
              

            if(PICC_IsNewCardPresent())
	   {
		   PICC_ReadCardSerial();
//
		   PICCDetails(&UID_t);
           
                   
            byte testAllFF[16];
memset(testAllFF, 0xFF, 16);
/*status = MIFARE_Write(0x60, 0x01, testAllFF);
printf("[DEBUG] ErrorReg: 0x%02X\n", MFRC522_ReadRegister(0x06));

if (status == MFRC522_STATUS_OK) 
           {
                printf("[DEBUG] Write Command Accepted by Card.\n");
                }
            else 
                         {
                    printf("[ERROR] Write failed! Card returned status: %d\n", status);
                    }
*/
//status = MIFARE_Read(PICC_CMD_MF_AUTH_KEY_A, 0x10, testBuffer);
// 1. Write "Hana Til" to the very first available data block
/*status = MIFARE_Write(0x60, 0x10, nameBuffer); 

if (status == MFRC522_STATUS_OK) 
           {
                printf("[DEBUG] Write Command Accepted by Card.\n");
                }
            else 
                         {
                    printf("[ERROR] Write failed! Card returned status: %d\n", status);
                    }*/

// 2. Read it back from the same block
status = MIFARE_Read(0x60, 0x10, testBuffer);
               printf("status main: %d \n", status);
               if (status == 0) { // MFRC522_STATUS_OK
    printf("Successfully Read Block 1!\n");
    printf("Hex Data: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", testBuffer[i]); // Print each byte in Hexadecimal
    }
    printf("\n");

    printf("String Data: ");
    for (int i = 0; i < 16; i++) {
        // Only print if it's a readable character (A-Z, 0-9, etc.)
        if (testBuffer[i] >= 32 && testBuffer[i] <= 126) {
            printf("%c", testBuffer[i]);
        } else {
            printf("."); // Show non-printable bytes as dots
        }
    }
    printf("\n---\n");
}








           MIFARE_StopSession(); 
           printf("Card processed and session closed.\n");
           // --- FUTURE SQL STEP ---
                // save_to_database(&UID_t);
		   }

        // 5. The "Linux Breathing" Step
        // 100ms (100,000us) keeps the CPU cool and the system responsive
        usleep(100000); 
    }

    //Cleanup (Close the file descriptor before exiting)
    close(spiFile);
    return 0;
}

















//gcc -I include/ main.c src/*.c -lrt  -o myprogram











/*status = MIFARE_Read(PICC_CMD_MF_AUTH_KEY_A, 0x00, testBuffer);
               printf("status main: %d \n", status);
               if (status == 0) { // MFRC522_STATUS_OK
    printf("Successfully Read Block 0!\n");
    printf("Hex Data: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", testBuffer[i]); // Print each byte in Hexadecimal
    }
    printf("\n");

    printf("String Data: ");
    for (int i = 0; i < 16; i++) {
        // Only print if it's a readable character (A-Z, 0-9, etc.)
        if (testBuffer[i] >= 32 && testBuffer[i] <= 126) {
            printf("%c", testBuffer[i]);
        } else {
            printf("."); // Show non-printable bytes as dots
        }
    }
    printf("\n---\n");
}*/




/*// 1. Create a test pattern (2 bytes)
byte testData[2] = {0x30, 0x00}; 
byte result[2];

// 2. Run the calculation
 status = PCD_CalculateCRC(testData, 2, result);

// 3. Print the result
if (status == MFRC522_STATUS_OK) {
    // For [0x30, 0x00], the standard MIFARE CRC is 0x02 0x98
    printf("[DEBUG] CRC Success! Result: %02X %02X\n", result[0], result[1]);
    
    if (result[0] == 0x02 && result[1] == 0x98) {
        printf("[RESULT] CRC MATCH! Hardware is perfect.\n");
    } else {
        printf("[RESULT] CRC MISMATCH! Check ModeReg (should be 0x3D).\n");
    }
} else {
    printf("[ERROR] CRC Calculation Timed Out! Check your SPI wiring.\n");
}*/

/*status =  MIFARE_Write(PICC_CMD_MF_AUTH_KEY_A, 0x10, nameBuffer);
           if (status == MFRC522_STATUS_OK) 
           {
                printf("[DEBUG] Write Command Accepted by Card.\n");
                }
            else 
                         {
                    printf("[ERROR] Write failed! Card returned status: %d\n", status);
                    }*/



                  /* if (status == MFRC522_STATUS_OK) {
    printf("READ SUCCESS! Hardware is fine, Write is blocked.\n");
} else {
    printf("READ FAILED! Code: %d, ErrorReg: 0x%02X\n", status, MFRC522_ReadRegister(ErrorReg));
}*/