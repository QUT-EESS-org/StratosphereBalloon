/*
 * Stratosphere Balloon
 * SBEEPROM.h
 * Purpose: Abstracts all EEPROM functions
 * Created: 31/12/2014
 * Author(s): Blake Fuller
 * Status: UNTESTED
 */ 
 
#ifndef SBEEPROM_H_
#define SBEEPROM_H_

#include <avr/eeprom.h>

#define WDT_CRASH_FLAG_LOC 0 	// Flag to see if WDT has been set
#define SD_POINTER_LOC 1		// Latest SD Card write location

void SBEEPROMWriteSDPoint(uint32_t val){
	eeprom_write_dword((uint32_t*) SD_POINTER_LOC, val);
}

uint32_t SBEEPROMReadSDPoint(void){
	return eeprom_read_dword((uint32_t*) SD_POINTER_LOC);
}

void SBEEPROMWriteWDTCrashFlag(uint8_t val){
	eeprom_write_byte((uint8_t*) WDT_CRASH_FLAG_LOC, val);
}

uint8_t SBEEPROMReadWDTCrashFlag(void){
	return eeprom_read_byte((uint8_t*) WDT_CRASH_FLAG_LOC);
}

#endif
