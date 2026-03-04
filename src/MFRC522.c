/*
 * MFRC522.c
 *
 *  Created on: Feb 19, 2026
 *      Author: Hana Tilouche
 *      Description: MFRC522 RFID User-Space Module driver for Raspberry pi.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <errno.h>
#include <unistd.h>
#include <time.h> 
#include "stdint.h"
#include "MFRC522.h"  
#include "SPI.h" 





// Member variables
Uid UID_t;



uint32_t millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void MFRC522_WriteRegister(PCD_Register_t reg, uint8_t value)
{
	const int NUM_BYTES = 2;

    uint8_t sendBuf[NUM_BYTES];
    sendBuf[0] = (REG_WRITE_OP_MASK | reg);
    sendBuf[1] = value;
    uint8_t rcvBuf[NUM_BYTES]; 
    memset(rcvBuf, 0, sizeof(rcvBuf));
    SPI_transfer(spiFile, sendBuf, rcvBuf, NUM_BYTES);

}

void MFRC522_WriteRegister_Multi(PCD_Register_t reg, byte count, uint8_t *value)
{
	int total_len = count + 1; // Address byte + data bytes
    uint8_t sendBuf[total_len];
    uint8_t rcvBuf[total_len]; // Dummy buffer for SPI_transfer signature

    // Set address with Write Mask (usually the mask is 0x7E or similar for MFRC522)
    sendBuf[0] = (reg | REG_WRITE_OP_MASK);

    // Copy all 'count' values into the buffer starting at index 1
    memcpy(&sendBuf[1], value, count);

    SPI_transfer(spiFile, sendBuf, rcvBuf, total_len);
}


uint8_t MFRC522_ReadRegister(PCD_Register_t reg)
{
    const int NUM_BYTES = 2;

    uint8_t sendBuf[NUM_BYTES];
    // Address byte (See section 8.1.2.3 of Doc)
    sendBuf[0] = (REG_READ_OP_MASK | (reg));
    // Second address byte (set 0 to stop reading) 
    sendBuf[1] = 0x00;

	uint8_t rcvBuf[NUM_BYTES]; 
    memset(rcvBuf, 0, sizeof(rcvBuf));

    SPI_transfer(spiFile, sendBuf, rcvBuf, NUM_BYTES);
	

    // First received byte will be blank
    return rcvBuf[1];
}
void MFRC522_ReadRegisterMulti(PCD_Register_t reg, byte count, byte *values, byte rxalign) {
    if (count == 0) return;

    // Prepare buffers. We need (count + 1) because the first byte sent is the address.
    uint8_t sendBuf[65] = {0}; 
    uint8_t recvBuf[65] = {0};
    uint8_t totalBytes = count + 1;

    // Set the address byte. MSB = 1 for Read.
    // We repeat the address in the buffer to keep the MFRC522 shifting the next FIFO byte.
    uint8_t address = 0x80 | reg;
    for (int i = 0; i < count; i++) {
        sendBuf[i] = address;
    }
    sendBuf[count] = 0; // Last byte is 0 to stop the burst

    // Perform the Linux SPI Transfer
    SPI_transfer(spiFile, sendBuf, recvBuf, totalBytes);
/*
	// --- THE DEBUG HERE ---
    printf("DEBUG SPI RAW [%d bytes]: ", totalBytes);
    for (int i = 0; i < totalBytes; i++) {
        printf("%02X ", recvBuf[i]);
    }
    printf("\n");
    // --------------------------
*/
    // Extract data from recvBuf. 
    uint8_t index = 0;

    if (rxalign) {
        uint8_t mask = (0xFF << rxalign) & 0xFF;
        uint8_t value = recvBuf[1]; // The first real data byte
        values[0] = (values[0] & ~mask) | (value & mask);
        index = 1;
    }

    // 5. Copy the rest of the bytes
    while (index < count) {
        values[index] = recvBuf[index + 1];
        index++;
    }
}

void MFRC522_ClearRegisterBitMask(PCD_Register_t reg, byte mask)
{
	byte tmp;
	tmp = MFRC522_ReadRegister(reg);
	MFRC522_WriteRegister(reg, tmp & (~mask));     //clear bit mask
}

void MFRC522_SetRegisterBitmask( PCD_Register_t reg, byte mask)
{
	byte tmp;
	tmp = MFRC522_ReadRegister(reg);
	MFRC522_WriteRegister(reg, tmp | (mask));     //set bit mask
}


void MFRC522_init()
{
	MFRC522_WriteRegister(ModeReg, 0x3D); 
	MFRC522_WriteRegister(TxModeReg, 0x00);
	MFRC522_WriteRegister(RxModeReg, 0x00);

	MFRC522_WriteRegister(ModWidthReg, 0x26);

	// When communicating with a PICC we need a timeout if something goes wrong.
	// f_timer = 13.56 MHz / (2*TPreScaler+1) where TPreScaler = [TPrescaler_Hi:TPrescaler_Lo].
	// TPrescaler_Hi are the four low bits in TModeReg. TPrescaler_Lo is TPrescalerReg.
	MFRC522_WriteRegister(TModeReg, 0x80);          // TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
	MFRC522_WriteRegister(TPrescalerReg, 0xA9);     // TPreScaler = TModeReg[3..0]:TPrescalerReg, ie 0x0A9 = 169 => f_timer=40kHz, ie a timer period of 25μs.
	//MFRC522_WriteRegister(TModeReg, 0x8D);      // TPrescaler_Hi = 0x0D (13)
//MFRC522_WriteRegister(TPrescalerReg, 0x3E);
	MFRC522_WriteRegister(TReloadRegH, 0x03);       // Reload timer with 0x3E8 = 1000, ie 25ms before timeout.
	MFRC522_WriteRegister(TReloadRegL, 0xE8);

//MFRC522_WriteRegister(RFCfgReg, (0x07 << 4)); 

	MFRC522_WriteRegister(TxASKReg, 0x40);// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
	MFRC522_WriteRegister(ModeReg, 0x3D); // Default 0x3F. Set the preset value for the CRC co processor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)		// Enable the antenna driver pins TX1 and TX2 (they were disabled by the reset)

	MFRC522_AnteenaOn();                  //Enable the Antenna Driver pins TX1 & TX2(They are disabled by reset)

}

void MFRC522_AnteenaOn()
{
	uint8_t An_x = MFRC522_ReadRegister(TxControlReg);
	if((An_x & 0x03) != 0x03)
	{
		MFRC522_WriteRegister(TxControlReg, (An_x | 0x03));
	}
}
void MFRC522Version()
{
	uint8_t v =  MFRC522_ReadRegister(VersionReg);
	
	 printf("MFRC522 Firmware Version: 0x%02X", v);

    switch(v)
    {
        case 0x88: printf(" = (Software Clone)\n"); break;
        case 0x90: printf(" = v0.0 (Old)\n"); break;
        case 0x91: printf(" = v1.0 (Standard)\n"); break;
        case 0x92: printf(" = v2.0 (Modern)\n"); break;
        case 0x12: printf(" = (Counterfeit/Fake Chip)\n"); break;
        case 0x00: 
        case 0xFF: printf(" = (No communication! Check wiring)\n"); break;
        default:   printf(" = (Unknown Version)\n"); break;
    }
}
MFRC522_statusCodes_t PCD_CalculateCRC(byte *data,  byte length,	byte *result)
{
MFRC522_WriteRegister(CommandReg, PCD_Idle);		// Stop any active command.
MFRC522_WriteRegister(DivIrqReg, 0x04);				// Clear the CRCIRq interrupt request bit
MFRC522_WriteRegister(FIFOLevelReg, 0x80);			// FlushBuffer = 1, FIFO initialization
MFRC522_WriteRegister_Multi(FIFODataReg, length, data);	// Write data to the FIFO
MFRC522_WriteRegister(CommandReg, PCD_CalcCRC);		// Start the calculation

// Wait for the CRC calculation to complete. Check for the register to
// indicate that the CRC calculation is complete in a loop. If the
// calculation is not indicated as complete in ~90ms, then time out
// the operation.
const uint32_t deadline = millis() + 89;

do {
	// DivIrqReg[7..0] bits are: Set2 reserved reserved MfinActIRq reserved CRCIRq reserved reserved
	byte n = MFRC522_ReadRegister(DivIrqReg);
	if (n & 0x04) {									// CRCIRq bit set - calculation done
		MFRC522_WriteRegister(CommandReg, PCD_Idle);	// Stop calculating CRC for new content in the FIFO.
		// Transfer the result from the registers to the result buffer
		result[0] = MFRC522_ReadRegister(CRCResultRegL);
		result[1] = MFRC522_ReadRegister(CRCResultRegH);
		return MFRC522_STATUS_OK;
	}
}
while ((millis()) < deadline);

// 89ms passed and nothing happened. Communication with the MFRC522 might be down.
return MFRC522_STATUS_TIMEOUT;
} // End PCD_CalculateCRC()
MFRC522_statusCodes_t MFRC522_CommunicateWithPICC(byte command,   //The command to execute
                                                      byte waitIRQ,
													  byte *sendData,
													  byte sendLen,
													  byte *backdata,
													  byte *backlen,
													  byte *validbits,
													  byte rxalign,
													bool checkCRC
													)
{
	byte txLastBits = validbits ? *validbits :0;
	byte bitFraming = (rxalign <<4) + txLastBits;    //RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

	MFRC522_WriteRegister(CommandReg, PCD_Idle);			// Stop any active command.
	MFRC522_WriteRegister(ComIrqReg, 0x7F);					// Clear all seven interrupt request bits

	// For PCD to PICC Communication!!
	MFRC522_WriteRegister(FIFOLevelReg, 0x80);				// FlushBuffer = 1, FIFO initialization
	MFRC522_WriteRegister_Multi(FIFODataReg, sendLen, sendData);	// Write sendData to the FIFO
    MFRC522_WriteRegister(BitFramingReg, bitFraming);		// Bit adjustments
	MFRC522_WriteRegister(CommandReg, command);				// Execute the command
	if(command == PCD_Transceive){
		MFRC522_SetRegisterBitmask(BitFramingReg, 0x80);     //StartSend=1, transmission of data starts
	}
  
	const uint32_t deadline = millis()+36;
	bool completed = false;
	do {
		byte n = MFRC522_ReadRegister(ComIrqReg);	// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
		if (n & waitIRQ) {					// One of the interrupts that signal success has been set.
			completed = true;
			break;
		}
		if (n & 0x01) {						// Timer interrupt - nothing received in 25ms
			return STATUS_TIMEOUT;
		}
	}
	while ((millis()) < deadline);

	//usleep(100000); 
	    	
//36ms and nothing happened. Communication with the MFRC522 might be down.
if (!completed) {
	return MFRC522_STATUS_TIMEOUT;
}

//Stop now if any errors except collisions were detected.
byte errorRegValue = MFRC522_ReadRegister(ErrorReg); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
if (errorRegValue & 0x13) {	 // BufferOvfl ParityErr ProtocolErr
    printf("Error: Communication failed!\n");
	return MFRC522_STATUS_ERROR;
}

byte _validbits = 0;
//If the caller wants data back, get it from the MFRC522.
if(backdata && backlen) {
	byte n = MFRC522_ReadRegister(FIFOLevelReg);

	if(n > *backlen){
		return MFRC522_STATUS_NO_ROOM;
	}

	*backlen = n;
	MFRC522_ReadRegisterMulti(FIFODataReg, n, backdata, rxalign);

	_validbits = MFRC522_ReadRegister(ControlReg) & 0x07;
	if(validbits){
		*validbits = _validbits;
			}
}
// Tell about collisions
if (errorRegValue & 0x08) {		// CollErr
	return MFRC522_STATUS_COLLISION;
}

// Perform CRC_A validation if requested.
if (backdata && backlen && checkCRC) {
	// In this case a MIFARE Classic NAK is not OK.
	if (*backlen == 1 && _validbits == 4) {
		return MFRC522_STATUS_MIFARE_NACK;
	}
	// We need at least the CRC_A value and all 8 bits of the last byte must be received.
	if (*backlen < 2 || _validbits != 0) {
		return MFRC522_STATUS_CRC_WRONG;
	}
	// Verify CRC_A - do our own calculation and store the control in controlBuffer.
	byte controlBuffer[2];
	MFRC522_statusCodes_t status = PCD_CalculateCRC(&backdata[0], *backlen - 2, &controlBuffer[0]);
	if (status != MFRC522_STATUS_OK) {
		return status;
	}
	if ((backdata[*backlen - 2] != controlBuffer[0]) || (backdata[*backlen - 1] != controlBuffer[1])) {
		return MFRC522_STATUS_CRC_WRONG;
	}
}

			return MFRC522_STATUS_OK;
}// END of MFRC522_CommunicateWithPICC;

MFRC522_statusCodes_t MFRC522_TransceiveData( byte *senddata,  //Pointer to the data to transfer to the FIFO
                                                  byte sendlen,       //Number of bytes to transfer to FIFO
												  byte *backdata,     //nullptr or pointer to buffer if data should be read back after executing the command
												  byte *backlen,      //In: Max no of bytes to write to *backdata. Out: The no of bytes returned.
												  byte *validbits,    //In/Out: The number of valid bits in the last byte
												  byte rxalign,
												  bool checkCRC       //In: True => The last 2 bytes of the response is assumed to be a CRC_A that must be validated
												  )
{
	byte waitIRQ = 0x30;


	usleep(100000); 
	return MFRC522_CommunicateWithPICC(PCD_Transceive, waitIRQ, senddata, sendlen, backdata, backlen, validbits, rxalign, checkCRC);
} //END of MFRC522_TransceiveData

MFRC522_statusCodes_t PICC_REQA_OR_WUPA(byte command, byte *bufferATQA, byte *buffersize)
{
	MFRC522_statusCodes_t status;
	byte validbits;
	if(bufferATQA == 0 || *buffersize < 2) { // As bufferATQA is a buffer to store the Answer to request, when command REQ is send by PCD.
	 return MFRC522_STATUS_NO_ROOM;                                                   //So this buffer should be pointing to some valid pointer and not to null pointer. And buffer
	}												   //size should be greater then 2 bytes, ATQA is of 2 bytes.

	MFRC522_ClearRegisterBitMask(CollReg,0x80);
	validbits = 7;

	status = MFRC522_TransceiveData(&command, 1, bufferATQA, buffersize, &validbits, 0, 0);

	if( status != MFRC522_STATUS_OK);
	return status;

	if(*buffersize !=2 || validbits != 0){
		return MFRC522_STATUS_ERROR;
	}

	return MFRC522_STATUS_OK;

}// END of PICC_REQA_OR_WUPA()

MFRC522_statusCodes_t PICC_RequestA(byte * bufferATQ, byte *buffersize)
{
	return PICC_REQA_OR_WUPA(PICC_CMD_REQA, bufferATQ, buffersize);
} // END of PICC_RequestA()
bool PICC_IsNewCardPresent(void)
{
	byte bufferATQ[2]; // We will be sending the Request command. That is in order to detect the PICCs which are in the operating field
	                     // PCD shall send repeated request commands. PCD will send REQ or WUP in any sequence to detect the PICCs.
						 //REQ commands are transmitted via short frame If PICC is in energizing field for PCD and gets powered up,
						 //it will start listening for valid REQ command. And transmits its ATQ(Answer to request).
						 //This answer to Request is stored in this buffer. ATQA is a 2 byte number that is returned by PICC.
	byte buffersize = sizeof(bufferATQ);
	// Reset baud rates
	MFRC522_WriteRegister(TxModeReg, 0x00);
	MFRC522_WriteRegister(RxModeReg, 0x00);
	// Reset ModWidthReg
	MFRC522_WriteRegister(ModWidthReg, 0x26);//38 in decimal

    MFRC522_statusCodes_t result = PICC_RequestA(bufferATQ, &buffersize);
    printf("Result: %d\n", result);
    return (result == MFRC522_STATUS_OK || result == MFRC522_STATUS_COLLISION);

} //END of PICC_IsNewCardPresent()

/**
 * Transmits SELECT/ANTICOLLISION commands to select a single PICC.
 * Before calling this function the PICCs must be placed in the READY(*) state by calling PICC_RequestA() or PICC_WakeupA().
 * On success:
 * 		- The chosen PICC is in state ACTIVE(*) and all other PICCs have returned to state IDLE/HALT. (Figure 7 of the ISO/IEC 14443-3 draft.)
 * 		- The UID size and value of the chosen PICC is returned in *uid along with the SAK.
 *
 * A PICC UID consists of 4, 7 or 10 bytes.
 * Only 4 bytes can be specified in a SELECT command, so for the longer UIDs two or three iterations are used:
 * 		UID size	Number of UID bytes		Cascade levels		Example of PICC
 * 		========	===================		==============		===============
 * 		single				 4						1				MIFARE Classic
 * 		double				 7						2				MIFARE Ultralight
 * 		triple				10						3				Not currently in use?
 *
 * @return MFRC522_STATUS_OK on success, STATUS_??? otherwise.
 */

MFRC522_statusCodes_t PICC_Select(	Uid *uid,			///< Pointer to Uid struct. Normally output, but can also be used to supply a known UID.
											byte validBits		///< The number of known UID bits supplied in *uid. Normally 0. If set you must also supply uid->size.
										 ) {
	bool uidComplete;
	bool selectDone;
	bool useCascadeTag;
	byte cascadeLevel = 1;
	MFRC522_statusCodes_t result;
	byte count;
	byte checkBit;
	byte index;
	byte uidIndex;					// The first index in uid->uidByte[] that is used in the current Cascade Level.
	int8_t currentLevelKnownBits;		// The number of known UID bits in the current Cascade Level.
	byte buffer[9];					// The SELECT/ANTICOLLISION commands uses a 7 byte standard frame + 2 bytes CRC_A
	byte bufferUsed;				// The number of bytes used in the buffer, ie the number of bytes to transfer to the FIFO.
	byte rxAlign;					// Used in BitFramingReg. Defines the bit position for the first bit received.
	byte txLastBits;				// Used in BitFramingReg. The number of valid bits in the last transmitted byte.
	byte *responseBuffer;
	byte responseLength;

	// Description of buffer structure:
	//		Byte 0: SEL 				Indicates the Cascade Level: PICC_CMD_SEL_CL1, PICC_CMD_SEL_CL2 or PICC_CMD_SEL_CL3
	//		Byte 1: NVB					Number of Valid Bits (in complete command, not just the UID): High nibble: complete bytes, Low nibble: Extra bits.
	//		Byte 2: UID-data or CT		See explanation below. CT means Cascade Tag.
	//		Byte 3: UID-data
	//		Byte 4: UID-data
	//		Byte 5: UID-data
	//		Byte 6: BCC					Block Check Character - XOR of bytes 2-5
	//		Byte 7: CRC_A
	//		Byte 8: CRC_A
	// The BCC and CRC_A are only transmitted if we know all the UID bits of the current Cascade Level.
	//
	// Description of bytes 2-5: (Section 6.5.4 of the ISO/IEC 14443-3 draft: UID contents and cascade levels)
	//		UID size	Cascade level	Byte2	Byte3	Byte4	Byte5
	//		========	=============	=====	=====	=====	=====
	//		 4 bytes		1			uid0	uid1	uid2	uid3
	//		 7 bytes		1			CT		uid0	uid1	uid2
	//						2			uid3	uid4	uid5	uid6
	//		10 bytes		1			CT		uid0	uid1	uid2
	//						2			CT		uid3	uid4	uid5
	//						3			uid6	uid7	uid8	uid9

	// Sanity checks
	if (validBits > 80) {
		return MFRC522_STATUS_INVALID;
	}

	// Prepare MFRC522
	MFRC522_ClearRegisterBitMask(CollReg, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.

	// Repeat Cascade Level loop until we have a complete UID.
	uidComplete = false;
	while (!uidComplete) {
		// Set the Cascade Level in the SEL byte, find out if we need to use the Cascade Tag in byte 2.
		switch (cascadeLevel) {
			case 1:
				buffer[0] = PICC_CMD_SEL_CL1;
				uidIndex = 0;
				useCascadeTag = validBits && uid->size > 4;	// When we know that the UID has more than 4 bytes
				break;

			case 2:
				buffer[0] = PICC_CMD_SEL_CL2;
				uidIndex = 3;
				useCascadeTag = validBits && uid->size > 7;	// When we know that the UID has more than 7 bytes
				break;

			case 3:
				buffer[0] = PICC_CMD_SEL_CL3;
				uidIndex = 6;
				useCascadeTag = false;						// Never used in CL3.
				break;

			default:
				return MFRC522_STATUS_INTERNAL_ERROR;
				break;
		}


		// How many UID bits are known in this Cascade Level?
		currentLevelKnownBits = validBits - (8 * uidIndex);
		if (currentLevelKnownBits < 0) {
			currentLevelKnownBits = 0;
		}
		// Copy the known bits from uid->uidByte[] to buffer[]
		index = 2; // destination index in buffer[]
		if (useCascadeTag) {
			buffer[index++] = PICC_CMD_CT; // buffer[3]
		}
		byte bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0); // The number of bytes needed to represent the known bits for this level.

		if (bytesToCopy) {
			byte maxBytes = useCascadeTag ? 3 : 4; // Max 4 bytes in each Cascade Level. Only 3 left if we use the Cascade Tag
			if (bytesToCopy > maxBytes) {
				bytesToCopy = maxBytes;
			}
			for (count = 0; count < bytesToCopy; count++) {
				buffer[index++] = uid->uidByte[uidIndex + count];
			}
		}
		// Now that the data has been copied we need to include the 8 bits in CT in currentLevelKnownBits
		if (useCascadeTag) {
			currentLevelKnownBits += 8;
		}

		// Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
		selectDone = false;
		while (!selectDone) {
			// Find out how many bits and bytes to send and receive.
			if (currentLevelKnownBits >= 32) { // All UID bits in this Cascade Level are known. This is a SELECT.
				buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole bytes
				// Calculate BCC - Block Check Character
				buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
				// Calculate CRC_A
				result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
 						printf("Result: %d\n", result);

				if (result != MFRC522_STATUS_OK) {
					return result;
				}
				txLastBits		= 0; // 0 => All 8 bits are valid.
				bufferUsed		= 9;
				// Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
				responseBuffer	= &buffer[6];
				responseLength	= 3;
			}
			else { // This is an ANTICOLLISION/SELECT
				//Serial.print(F("ANTICOLLISION: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
				txLastBits		= currentLevelKnownBits % 8;
				count			= currentLevelKnownBits / 8;	// Number of whole bytes in the UID part.
				index			= 2 + count;					// Number of whole bytes: SEL + NVB + UIDs
				buffer[1]		= (index << 4) + txLastBits;	// NVB - Number of Valid Bits

				bufferUsed		= index + (txLastBits ? 1 : 0);
				// Store response in the unused part of buffer
				responseBuffer	= &buffer[index];
				responseLength	= sizeof(buffer) - index;
			}

			// Set bit adjustments
			rxAlign = txLastBits;											// Having a separate variable is overkill. But it makes the next line easier to read.
			MFRC522_WriteRegister(BitFramingReg, (rxAlign << 4) + txLastBits);	// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

			// Transmit the buffer and receive the response.
			result = MFRC522_TransceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign, 0);

			if (result == MFRC522_STATUS_COLLISION) { // More than one PICC in the field => collision.
				byte valueOfCollReg = MFRC522_ReadRegister(CollReg); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
				if (valueOfCollReg & 0x20) { // CollPosNotValid
					return MFRC522_STATUS_COLLISION; // Without a valid collision position we cannot continue
				}
				byte collisionPos = valueOfCollReg & 0x1F; // Values 0-31, 0 means bit 32.
				if (collisionPos == 0) {
					collisionPos = 32;
				}
				if (collisionPos <= currentLevelKnownBits) { // No progress - should not happen
					return MFRC522_STATUS_INTERNAL_ERROR;
				}
				// Choose the PICC with the bit set.
				currentLevelKnownBits	= collisionPos;
				count			= currentLevelKnownBits % 8; // The bit to modify
				checkBit		= (currentLevelKnownBits - 1) % 8;
				index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
				buffer[index]	|= (1 << checkBit);
			}
			else if (result != MFRC522_STATUS_OK) {
				return result;
			}
			else { // MFRC522_STATUS_OK
				if (currentLevelKnownBits >= 32) { // This was a SELECT.
					selectDone = true; // No more anticollision
					// We continue below outside the while.
				}
				else { // This was an ANTICOLLISION.
					// We now have all 32 bits of the UID in this Cascade Level
					currentLevelKnownBits = 32;
					// Run loop again to do the SELECT.
				}
			}
		} // End of while (!selectDone)

		// We do not check the CBB - it was constructed by us above.

		// Copy the found UID bytes from buffer[] to uid->uidByte[]
		index			= (buffer[2] == PICC_CMD_CT) ? 3 : 2; // source index in buffer[]
		bytesToCopy		= (buffer[2] == PICC_CMD_CT) ? 3 : 4;
		for (count = 0; count < bytesToCopy; count++) {
			uid->uidByte[uidIndex + count] = buffer[index++];
		}

		// Check response SAK (Select Acknowledge)
		if (responseLength != 3 || txLastBits != 0) { // SAK must be exactly 24 bits (1 byte + CRC_A).
			return MFRC522_STATUS_ERROR;
		}
		// Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those bytes are not needed anymore.
		result = PCD_CalculateCRC(responseBuffer, 1, &buffer[2]);
		if (result != MFRC522_STATUS_OK) {
			return result;
		}
		if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2])) {
			return MFRC522_STATUS_CRC_WRONG;
		}
		if (responseBuffer[0] & 0x04) { // Cascade bit set - UID not complete yes
			cascadeLevel++;
		}
		else {
			uidComplete = true;
			uid->sak = responseBuffer[0];
		}
	} // End of while (!uidComplete)

	// Set correct uid->size
	uid->size = 3 * cascadeLevel + 1;

	return MFRC522_STATUS_OK;
} // End PICC_Select()

bool PICC_ReadCardSerial(void)
{
	MFRC522_statusCodes_t result = PICC_Select(&UID_t, 0);
	return (result == MFRC522_STATUS_OK);
}



void PICCDetails(Uid *uid)
{
    //  Print the UID Header
    printf("Card UID: ");

    //  Loop through the UID bytes
    for (byte i = 0; i < uid->size; i++) {
        printf("%02X ", uid->uidByte[i]);
    }
    printf("\n");

    //  Print the SAK (Select Acknowledge) byte
    printf("Card SAK: %02X\n", uid->sak);
}
MFRC522_statusCodes_t MIFARE_Authenticate(PICC_Command_t key, byte blockAddr, Uid *uid)
{
	byte irqReg;
	byte MFCryptoValue;
	byte data[12];
	byte length = sizeof(data);

	data[0] = key; 
    data[1] = blockAddr;
    memset(&data[2], 0xFF, 6);
	memcpy(&data[8], UID_t.uidByte, 4); //uid->uidByte

	/*for (byte i = 8; i < length; i++) {
        data[i] = uid->uidByte[i - 8]; 
    }*/

	MFRC522_WriteRegister(CommandReg, PCD_Idle);         //stop any current activity
	MFRC522_WriteRegister(FIFOLevelReg, 0x80);			// FlushBuffer = 1, FIFO initialization
	MFRC522_WriteRegister_Multi(FIFODataReg, length, data);	// Write data to the FIFO
	MFRC522_WriteRegister(ComIrqReg, 0x7F);					// Clear all seven interrupt request bits
	MFRC522_WriteRegister(CommandReg, PCD_MFAuthent);		// Start the authentication
	const uint32_t deadline = millis()+36;
	do 
	{
    	irqReg = MFRC522_ReadRegister(ComIrqReg);
		if (irqReg & 0x11) break; 

	} 
	while ((millis()) < deadline);
	if(irqReg & 0x01)  // TimerIRq bit , if the TimerIRq bit  is set the card is not found and the reader gave up waiting for it 
	{
		return MFRC522_STATUS_TIMEOUT;
	}
	else if(irqReg & 0x10)  // we can check the authentication now
	{
		MFCryptoValue = MFRC522_ReadRegister(Status2Reg); //  indicates that the MIFARE Crypto1 unit is switched on andtherefore all data communication with the card is encrypted can only be set to logic 1 by a successful execution of the MFAuthent command 
		if (MFCryptoValue & 0x08) 
		{
			// Success! Crypto1On is active.									
			return MFRC522_STATUS_AUTHEN;
		}
	}
	return MFRC522_STATUS_NOT_AUTHEN;

}
MFRC522_statusCodes_t MIFARE_WriteDataRaw(byte *data, byte len) {
    // 1. CLEAR interrupts
    MFRC522_WriteRegister(ComIrqReg, 0x7F);

    // 2. FLUSH FIFO
    MFRC522_WriteRegister(FIFOLevelReg, 0x80);

    // 3. LOAD the 18 bytes (16 data + 2 CRC)
    MFRC522_WriteRegister_Multi(FIFODataReg, len, data);

    // 4. FORCE BitFraming to 0x00 (The "Secret" fix)
    // This ensures we send all 8 bits of every byte!
    MFRC522_WriteRegister(BitFramingReg, 0x00);

    // 5. START the transceive
    MFRC522_WriteRegister(CommandReg, PCD_Transceive);
    MFRC522_SetRegisterBitmask(BitFramingReg, 0x80); // StartSend = 1

    // 6. WAIT (As per your Table 25: 10ms)
    usleep(10000); 

    // 7. Check for ACK (0x0A) or Error
    byte irq = MFRC522_ReadRegister(ComIrqReg);
    if (irq & 0x01) return STATUS_TIMEOUT; // Timer expired
    
    return MFRC522_STATUS_OK;
}

MFRC522_statusCodes_t MIFARE_Write(PICC_Command_t key, byte blockAddr, byte *buffer)
{ 
	MFRC522_statusCodes_t result;
	MFRC522_statusCodes_t status;

	byte commandBuffer[4];
	commandBuffer[0] = PICC_CMD_MF_WRITE; // MIFARE_WRITE command
    commandBuffer[1] = blockAddr;

	result = MIFARE_Authenticate(key, blockAddr, &UID_t);
	if(result != MFRC522_STATUS_AUTHEN )
	{
		//printf("Authentication failed\n");
		return result;
	}
	/*byte status2 = MFRC522_ReadRegister(Status2Reg);
	if (!(status2 & 0x08)) {
    printf("[DEBUG] Crypto bit dropped! Authentication lost.\n");
}*/
    PCD_CalculateCRC(commandBuffer, 2, &commandBuffer[2]); 

	status = MFRC522_TransceiveData(commandBuffer, 4, NULL, 0, NULL, 0, 0); // WE send write command with block adress we want to write to, 
	if (status != MFRC522_STATUS_OK)
	{
		/*printf("command key failed\n");
		byte errorReg = MFRC522_ReadRegister(ErrorReg); // Register 0x06
    printf("[DEBUG] ErrorReg Value: 0x%02X\n", errorReg);*/
    printf("status key: %d", status);

    return status;
    }
	byte dataBuffer[18];
    memcpy(dataBuffer, buffer, 16); // Copy your "Hana Til"
    
    // Manually calculate and "fill" the last 2 bytes of the 18-byte block
    PCD_CalculateCRC(dataBuffer, 16, &dataBuffer[16]);

		usleep(10000);		
		status = MIFARE_WriteDataRaw(dataBuffer, 18);		

	//status = MFRC522_TransceiveData(buffer, 18, NULL, 0, NULL, 0, 0);
	if (status != MFRC522_STATUS_OK)
	{
    printf("transceive data didn't really sent: %d", status);

    return status;
    }
	printf("transceive data really sent: %d", status);

    return status;
}

MFRC522_statusCodes_t MIFARE_Read(PICC_Command_t key, byte blockAddr, byte *backData)
{ 
	MFRC522_statusCodes_t result;
	MFRC522_statusCodes_t status;
	byte commandBuffer [4];
	commandBuffer[0] = PICC_CMD_MF_READ; // MIFARE_WRITE command
    commandBuffer[1] = blockAddr;
	byte validBits = 0; 
    byte rxAlign = 0;

	result = MIFARE_Authenticate(key, blockAddr, &UID_t);
	if(result != MFRC522_STATUS_AUTHEN )
	{
		return result;
	}
	PCD_CalculateCRC(commandBuffer, 2, &commandBuffer[2]); 
	byte receivedLen = 18; 
	status = MFRC522_TransceiveData(commandBuffer, 4, backData, &receivedLen, &validBits, rxAlign, 0); 					
   //printf("status read: %d \n", status);
    return status;
}
MFRC522_statusCodes_t MIFARE_StopSession() 
{
    MFRC522_statusCodes_t status;
    byte haltBuffer[2];
    haltBuffer[0] = PICC_CMD_HALT; // 0x50
    haltBuffer[1] = 0;              // 0x00

   
    MFRC522_TransceiveData(haltBuffer, 2, NULL, 0, NULL, 0, false);

    // 2. Tell the READER to stop the encryption (Crypto1)
    // This clears bit 3 (0x08) of the Status2Reg.
    MFRC522_ClearRegisterBitMask(Status2Reg, 0x08);

    return MFRC522_STATUS_OK;
}


