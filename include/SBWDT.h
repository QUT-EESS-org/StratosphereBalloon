/*
 * Stratosphere Balloon
 * SBWDT.h
 * Purpose: Abstracts all WatchDog Timer functions
 * Created: 30/12/2014
 * Author(s): Blake Fuller
 * Status: UNTESTED
 */ 

#include <avr/WDT.h>
#include "SBEEPROM.h"
#include "SBCtrl.h"
 
 #define WDT16ms 
 #define WDT32ms 	(1<<WDP0)
 #define WDT64ms 	(1<<WDP1)
 #define WDT125ms 	(1<<WDP1)|(1<<WDP0)
 #define WDT250ms	(1<<WDP2)
 #define WDT500ms	(1<<WDP2)|(1<<WDP0)
 #define WDT1000ms	(1<<WDP2)|(1<<WDP1)
 #define WDT2000ms	(1<<WDP2)|(1<<WDP1)|(1<<WDP0)
 #define WDT4000ms	(1<<WDP3)
 #define WDT8000ms	(1<<WDP3)|(1<<WDP0)
 
volatile uint32_t SDWriteLocPoint;

 // See from page 63 of the AT90USB1286 datasheet
 
 void SBWDTEn(void){
	cli(); 							// Disable interrupts - following instructions are clock cycle dependant
	wdt_reset(); 					// Reset WDT
	WDTCSR = (1<<WDCE)|(1<<WDE); 	// Signal WDT config change
	WDTCSR =(1<<WDIE)|(1<<WDE)|  	// Enable WDT in Interrupt and System Reset mode
			WDT4000ms;				// Set prescaler to enable 4.0s WDT time period
	sei(); 							// Enable interrupts
}

void SBWDTDis(void){
	cli(); 							// Disable interrupts - following instructions are clock cycle dependant
	wdt_reset(); 					// Reset WDT
	MCUSR &= ~(1<<WDRF); 			// Disable WDT multiple reset fallback bit
	WDTCSR |= (1<<WDCE)|(1<<WDE); 	// Signal WDT config change
	WDTCSR = 0; 					// Disable WDT
	sei(); 							// Enable interrupts
}

ISR(WDT_vect){
	if(DEBUG_MODE){
		for (uint8_t i=0;i<4;i++) {
			//LED ON
			RLEDPORT|=(1<<RLEDBIT);
			//~0.1s delay
			_delay_ms(20);
			//LED OFF
			RLEDPORT&=~(1<<RLEDBIT);
			_delay_ms(80);
		}
	}
	else{
		SBEEPROMUpdateCrashCounter();
		SBEEPROMWriteWDTCrashFlag(1);
		SBEEPROMWriteNumSamples(SBData.numSamples);
		SBEEPROMWriteSDPoint(++SBData.SDLoc); 
	}
}
