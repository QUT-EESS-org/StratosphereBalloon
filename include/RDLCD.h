/*
 * libRobotDev
 * RDLCD.h
 * Purpose: Abstracts all LCD functions
 * Created: 05/12/2014
 * Author(s): Thomas Tutin, Arda Yilmaz
 * Status: UNTESTED
 */

#ifndef RDLCD_H_
#define RDLCD_H_

#include <avr/io.h>

#define RDSPI_MASTER 1
#include "RDSPI.h"

// Ascii font data
#include "RDASCIIFont.h"

// Font width
#define RDLCD_FONT_W    5

// LCD pins
#define RDLCD_CS        PC0
#define RDLCD_RST       PC1
#define RDLCD_DC        PC3
#define RDLCD_PORT      PORTC

// LCD Command/Data
#define RDLCD_C         0
#define RDLCD_D         1

// LCD Commands
#define LCD_BSC_FN      0x20
#define LCD_EXT_FN      0x21

#define LCD_BLANK       0x08
#define LCD_NORM        0x0C
#define LCD_BLACK       0x09
#define LCD_INVRT       0x0D

#define	LCD_SET_CONTRAST	0x80

#define LCD_SET_X       0x80
#define LCD_SET_Y       0x40

// SPI Parameters
#define LCD_CLK_PS	    0

// LCD Parameters
#define RDLCD_W         84
#define RDLCD_H         48
#define RDLCD_CNTR_X    RDLCD_W / 2
#define RDLCD_CNTR_Y    RDLCD_H / 2
#define RDLCD_ROW_H     8
#define RDLCD_DEFAULT_CONTRAST	0x3F
#define RDLCD_HIGH_CONTRAST		0x4F

#define RDLCD_SELECT()      RDLCD_PORT &= ~(1 << RDLCD_CS)
#define RDLCD_RELEASE()     RDLCD_PORT |= (1 << RDLCD_CS)

void RDLCDWrite(uint8_t byte, uint8_t dc) {
	
	// Set data/command mode
	RDLCD_PORT = (RDLCD_PORT & ~(1 << RDLCD_DC)) | (dc << RDLCD_DC);
	
	// Write byte to SPI
	RDSPIRWByte(byte, 7);
}

void RDLCDPosition(uint8_t x, uint8_t y) {
    
    RDLCD_SELECT();
    
    if((x < RDLCD_W) & (y < (RDLCD_ROW_H))) {
		
        RDLCDWrite(LCD_SET_X | x, RDLCD_C);
        RDLCDWrite(LCD_SET_Y | y, RDLCD_C);
    }
    
    RDLCD_RELEASE();
}

void RDLCDClear(void) {
    
    RDLCD_SELECT();
    for (int i = 0; i < RDLCD_W * RDLCD_ROW_H; i++) {
		
        RDLCDWrite(0x00, RDLCD_D);
    }
    RDLCD_RELEASE();
}

void RDLCDBlank(void) {

    RDLCD_SELECT();
    
	// Use basic instruction set
	RDLCDWrite(LCD_BSC_FN, RDLCD_C);
	
	// Blank screen
	RDLCDWrite(LCD_BLANK, RDLCD_C);
    
    RDLCD_RELEASE();
}

void RDLCDBlack(void) {
	
    RDLCD_SELECT();
    
	// Use basic instruction set
	RDLCDWrite(LCD_BSC_FN, RDLCD_C);
	
	// Blank screen
	RDLCDWrite(LCD_BLACK, RDLCD_C);
    
    RDLCD_RELEASE();
}

void RDLCDInvert(void) {
	
    RDLCD_SELECT();
    
	// Use basic instruction set
	RDLCDWrite(LCD_BSC_FN, RDLCD_C);
	
	// Blank screen
	RDLCDWrite(LCD_INVRT, RDLCD_C);
    
    RDLCD_RELEASE();
}

void RDLCDNormal(void) {
	
    RDLCD_SELECT();
    
	// Use basic instruction set
	RDLCDWrite(LCD_BSC_FN, RDLCD_C);
	
	// Blank screen
	RDLCDWrite(LCD_NORM, RDLCD_C);
    
    RDLCD_RELEASE();
}

void RDLCDSetContrast(uint8_t contrast) {

    RDLCD_SELECT();
	
    // Use extended instruction set
    RDLCDWrite(LCD_EXT_FN, RDLCD_C);
	
	// Set contrast 0 - 0x7f
    RDLCDWrite(LCD_SET_CONTRAST | (0x7f & contrast), RDLCD_C);
    
    RDLCD_RELEASE();
}

void RDLCDCharacter(unsigned char character) {
	
    RDLCD_SELECT();
    
    // Clear padding between each character
    RDLCDWrite(0x00, RDLCD_D);
    
    for (int i = 0; i < RDLCD_FONT_W; i++) {
		
        RDLCDWrite(pgm_read_byte(&ASCII[character - 0x20][i]), RDLCD_D);
    }
    
    RDLCDWrite(0x00, RDLCD_D);
    
    RDLCD_RELEASE();
}

void RDLCDString(unsigned char *characters) {
    
    while (*characters != '\0') {
		
        RDLCDCharacter(*(characters++));
    }
}

void RDLCDInit(void) {
	
	// Init LCD control pins, pull CS and RST high
	DDRC |= (1 << RDLCD_CS) | (1 << RDLCD_RST) | (1 << RDLCD_DC);
	RDLCD_PORT |= (1 << RDLCD_CS) | (1 << RDLCD_RST);
	
    RDLCD_SELECT();
	
    // Reset LCD
	RDLCD_PORT &= ~(1 << RDLCD_RST);
	RDLCD_PORT |= (1 << RDLCD_RST);
	
	// Init SPI - little endian, CPOL = 0, CPHA = 0
	RDSPIInit(0,0);
    
    // Set default contrast
    RDLCDSetContrast(RDLCD_HIGH_CONTRAST);
    
    // Set LCD to normal mode
    RDLCDNormal();
    
    // Reset the LCD position
    RDLCDPosition(0,0);
    
    RDLCD_RELEASE();
}

#endif // RDLCD_H_
