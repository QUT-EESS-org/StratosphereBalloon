#ifndef GC_H_
#define GC_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include "SBCtrl.h"
uint16_t count = 0;

void SBGeigerInit(void) {

	// Trigger interrupt on falling edge
	EICRB |= (1 << ISC41);
	// GC attached to INT4 pin
	EIMSK |= (1 << INT4);
	sei();
}

uint16_t SBGeigerRead(void) {

	uint16_t cpm = count;
	count = 0;
	return cpm;
}

void SBGeigerToLCD(void){
	unsigned char geigerStr[5];
	
	uint16_t cpm = SBGeigerRead();
	
	itoa(cpm, geigerStr, 10);
	
	RDLCDPosition(0, 2);					
	RDLCDString((unsigned char*) "Rad:     ");
	RDLCDPosition(35, 2);
	RDLCDString(geigerStr);
	RDLCDString((unsigned char*) "cpm");
}

ISR(INT4_vect) {
	
	count++;
}

#endif // GC_H_