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
#define SD_POINTER_LOC 5		// Latest SD Card write location
#define CRASH_COUNTER_LOC 10		// Number of WDT crashes
#define NUM_SAMPLES_LOC 20		// Number of samples taken

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

int16_t SBEEPROMReadCrashCount(void){
	return eeprom_read_word((uint16_t*) CRASH_COUNTER_LOC);
}

void SBEEPROMUpdateCrashCounter(void){
	int16_t prevVal = SBEEPROMReadCrashCount();
	eeprom_write_word((uint16_t*) CRASH_COUNTER_LOC, ++prevVal);
}

void SBEEPROMResetCrashCounter(uint16_t val){
	eeprom_write_word((uint16_t*) CRASH_COUNTER_LOC, val);
}

void SBEEPROMWriteNumSamples(int16_t val){
	eeprom_write_word((uint16_t*) NUM_SAMPLES_LOC, val);
}

int16_t SBEEPROMReadNumSamples(void){
	return eeprom_read_word((uint16_t*) NUM_SAMPLES_LOC);
}

#endif
