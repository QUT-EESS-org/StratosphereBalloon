/*
 * libRobotDev
 * RDSD.h
 * Purpose: Abstracts all SD functions
 * Created: 30/12/2014
 * Author(s): Thomas Tutin, Arda Yilmaz
 * Status: UNTESTED
 */ 

#ifndef RDSD_H_
#define RDSD_H_

#include <avr/io.h>
#include "RDSPI.h"

/* RDSPI defines */
#define RDSPI_MASTER    1

/* RDSD defines */
// Hardware Interfaces
#define RDSD_CS_DDR     DDRC
#define RDSD_CS_PORT    PORTC
#define RDSD_CS_PIN     PC0

#define RDSD_SELECT()   RDSD_CS_PORT &= ~(1 << RDSD_CS_PIN)
#define RDSD_RELEASE()  RDSD_CS_PORT |= (1 << RDSD_CS_PIN)

// SD Flags
#define SD_SUCCESS              0x00
#define SD_IDLE                 0x01
#define SD_ILGL                 0x04
#define SD_W_SUCCESS            0x05
#define SD_BUF_STRT             0xFE

#define SD_VTGE                 0x01
#define SD_PTRN                 0x55
#define SD_HCS                  0x40000000

// etc
#define SET_SD_CAP(n)           SDParam = ((SDParam & ~1) | (n & 1))
#define SET_SD_VER(n)           SDParam = ((SDParam & ~6) | ((n & 3) << 1))
#define SD_CAP                  (SDParam & 1)
#define SD_VER                  ((SDParam & 6) >> 1)
#define SD_SC                   0x00
#define SD_HXC                  0x01

// SPI Mode Commands
#define CMD_GO_IDLE_STATE       0
#define CMD_SEND_OP_COND        1
#define CMD_SEND_IF_COND        8
#define CMD_SEND_CSD            9
#define CMD_SEND_CID            10
#define CMD_STOP_TRANSMISSION   12
#define CMD_SEND_STATUS         13
#define CMD_SET_BLOCKLEN        16
#define CMD_READ_SINGLE_BLOCK   17
#define CMD_READ_MULT_BLOCK     18
#define CMD_WRITE_SINGLE_BLOCK  24
#define CMD_WRITE_MULT_BLOCK    25
#define CMD_PROGRAM_CSD         27
#define CMD_SET_WRITE_PROT      28
#define CMD_CLR_WRITE_PROT      29
#define CMD_SEND_WRITE_PROT     30
#define ACMD_SD_SEND_OP_COND    41
#define CMD_APP_CMD             55
#define CMD_GEN_CMD             56
#define CMD_READ_OCR            58
#define CMD_CRC_ON_OFF          59

uint8_t SDParam = 0;

/*
    Generates the CRC (cyclic redundancy check)
    This code was borrowed from another library 
*/
static uint8_t RDSDCRCgen(uint8_t * buffer, uint8_t length, uint8_t crc){
    uint8_t i, a;
    uint8_t data;

    for(a = 0; a < length; a++){
        data = buffer[a];
        for(i = 0; i < 8; i++){
            crc <<= 1; // crc = crc << 1; equivalent
            if( (data & 0x80) ^ (crc & 0x80) ) crc ^= 0x09;
            data <<= 1;
        }
    }
    return crc & 0x7F;
}

static uint8_t RDSDReadByte(void) {
    
    return RDSPIRWByte(0xFF, 2);
}

static uint8_t RDSDWaitResponse(void) {

    uint8_t byte;
    uint8_t i = 0;
    
    // Polls the device for OK
    do {
        byte = RDSDReadByte();
    } while((i++ < 32) && (byte == 0xFF));
    
    return byte;
}

static int8_t RDSDResponseR3(uint8_t *response, uint8_t retry) {

    // Read first byte of response
    do {
        *response = RDSDWaitResponse();
        if (!retry) return -1;      // SD timed out
        retry--;
    }
    while (*response == 0xFF);
    // Read remaining bytes
    for (uint8_t i = 1; i < 5; i++) response[i] = RDSDWaitResponse();
    // Send extra 0xFF
    RDSDReadByte();
    
    return 0;
}

static int8_t RDSDResponseR7(uint8_t *response, uint8_t retry) {

    // Read first byte of response
    do {
        *response = RDSDWaitResponse();
        if (!retry) return -1;      // SD timed out
        retry--;
    }
    while (*response == 0xFF);
    // Read remaining bytes
    for (uint8_t i = 1; i < 4; i++) response[i] = RDSDWaitResponse();
    // Send extra 0xFF
    RDSDReadByte();
    
    return 0;
}

static inline void RDSDSendCommand(uint8_t command, uint32_t param) {
    
    uint8_t buffer[6] = { 
        command | 0x40,
        (param >> 24),
        (param >> 16),
        (param >> 8),
        param,
        0
    };
    
    buffer[5] = (RDSDCRCgen(buffer,5,0) << 1) | 1;
    
    for (uint8_t i= 0; i < sizeof(buffer); i++) {
        RDSPIRWByte(buffer[i], 2);
    }
}

static inline void RDSDSendACommand(uint8_t command, uint32_t param) {
    
    RDSDSendCommand(CMD_APP_CMD,0);
    RDSDWaitResponse();
    RDSDReadByte();
    RDSDSendCommand(command,param);
}

static int8_t RDSDCapabilities(void) {
    
    uint16_t i;
    uint8_t response[5];
    
    // Set CS pin as output
    RDSD_CS_DDR |= (1 << RDSD_CS_PIN);
    
    // Pull CS low
    RDSD_SELECT();
    
    // Initialise SPI (little endian, rising sample | falling setup)
    RDSPIInit(0,0);
    
    // Send 80 clock pulses to wake from sleep
    for (i = 0; i < 10; i++) RDSDReadByte();
    
    // Send soft reset command (CMD0)
    RDSDSendCommand(CMD_GO_IDLE_STATE, 0);
    
    // Wait for CMD0 response
    if (RDSDWaitResponse() != SD_IDLE) goto initFailed;
    
    // Send extra 0xFF
    RDSDReadByte();
    
    // Check card version (CMD8)
    RDSDSendCommand(CMD_SEND_IF_COND, (SD_VTGE << 8) | (SD_PTRN));
    
    // Wait for CMD8 response
    if (RDSDResponseR3(response, 8) == -1) goto initFailed;
    
    // Check firmware version
    if ((response[0] == SD_IDLE) && (response[3] == SD_VTGE) &&
    (response[4] == SD_PTRN))               SET_SD_VER(2);
    
    else if (*response == (SD_ILGL | SD_IDLE))  SET_SD_VER(1);
    
    else goto initFailed;     // Unsupported card
    
    // Tell SD card we support HC/XC (ACMD41)
    i = 1024;
    do {
        RDSDReadByte();
        RDSDSendACommand(ACMD_SD_SEND_OP_COND, SD_HCS);
        if (!i) goto initFailed;      // SD timed out
        i--;
    }
    while(RDSDWaitResponse() & SD_IDLE);
    
    // Send extra 0xFF
    RDSDReadByte();
    
    // If not a version 1 card
    if (SD_VER != 1) {
    
        // Check SD card capacity (CMD58)
        RDSDSendCommand(CMD_READ_OCR, 0);
        
        RDSDResponseR7(response, 32);

        if ((response[0] == SD_SUCCESS) && (response[1] & 0x80)) {
            
            SET_SD_CAP(response[1] >> 6);
        }
    }
    
    // Set R/W block length to 512 bytes
    RDSDSendCommand(CMD_SET_BLOCKLEN, 512);
    
    if (RDSDWaitResponse() != SD_SUCCESS) goto initFailed;

    RDSDReadByte();
    // Pull CS high
    RDSD_RELEASE();
    return 0;
    
initFailed:
    RDSDReadByte();
    // Pull CS high
    RDSD_RELEASE();
    return -1;  
}

int8_t RDSDReadBuffer(uint32_t sector, uint16_t offset, uint8_t *buffer, uint32_t bufferLength) {
    
    RDSD_SELECT();
    
    uint16_t i = 1024;
    do {
        RDSDReadByte();
        if (!i) goto failed;      // SD timed out
        i--;
    }
    while(RDSDWaitResponse() == 0);
    
    if (SD_CAP == SD_SC) RDSDSendCommand(CMD_READ_SINGLE_BLOCK, (sector << 9));
    else RDSDSendCommand(CMD_READ_SINGLE_BLOCK, sector);
    
    // Wait for ACK
    if (RDSDWaitResponse() != SD_SUCCESS) goto failed;
    
    // Wait for buffer start flag
    if (RDSDWaitResponse() != SD_BUF_STRT) goto failed;
    
    // Skip to offset
    for (i = 0; i < offset; i++)
        RDSDReadByte();
    
    // Write data to buffer
    for (i = 0; i < bufferLength; i++)
        buffer[i] = RDSDReadByte();
    
    // Skip to offset
    for (i += offset; i < 512; i++)
        RDSDReadByte();
    
    // Skip checksum and send extra 0xFF
    for (i = 0; i < 3; i++)
        RDSDReadByte();
        
    RDSD_RELEASE();
    return 0;
    
failed:
    // Pull CS high
    RDSD_RELEASE();
    return -1;
}

int8_t RDSDWriteBuffer(uint32_t sector, uint8_t *buffer) {
    
    RDSD_SELECT();
    
    if (SD_CAP == SD_SC) RDSDSendCommand(CMD_WRITE_SINGLE_BLOCK, (sector << 9));
    else RDSDSendCommand(CMD_WRITE_SINGLE_BLOCK, sector);
    
    // Wait for ACK
    if (RDSDWaitResponse() != SD_SUCCESS) goto failed;
    
    // Send for buffer start flag
    RDSPIRWByte(SD_BUF_STRT, 2);
    
    uint16_t i = 0;
    
    // Send data
    for (i = 0; i < 512; i++)
        RDSPIRWByte(buffer[i], 2);
    
    // Send stuffed CRC
    for (i = 0; i < 1; i++)
        RDSDReadByte();
    
    if ( (RDSDWaitResponse() & 0x1F) != SD_W_SUCCESS ) goto failed;      
    
    RDSDReadByte();
    RDSD_RELEASE();
    return 0;
   
failed:
    RDSDReadByte();
    // Pull CS high
    RDSD_RELEASE();
    return -1;    
}

int8_t RDSDInit(void) {

    if (RDSDCapabilities() == -1) return -1;
    
    return 0;
}

#endif