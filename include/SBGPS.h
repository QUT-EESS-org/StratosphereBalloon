#ifndef SBGPS_H_
#define SBGPS_H_

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "RDUART.h"

#define     NMEA_STR_SIZE   80
#define		GPS_FIX_PIN		4

void GPSInit(void);
uint8_t GPSGetLocation(double location[], uint8_t *timeH, uint16_t *timeL);

static char GPSCheckFix(void);
static void GPSGetSentence(char sentence[]);
static void GPSParseSentence (char sentence[], double location[], uint8_t *timeH, uint16_t *timeL);


void GPSInit(void) {
    
	// Enable GPS UART
    RDUARTInit(9600);

    char i = 0;
    // NMEA Command - Set update rate
    unsigned char command[] = "$PMTK220,1000*1F\r\n";
        
    // 1Hz update
    for (i = 0; command[i] != '\0'; ++i) {
		RDUARTSendChar(command[i]);
	}
	
	// NMEA Command - Set outputs
	unsigned char command2[] = "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";
	
	// Only enable GGA output
    for (i = 0; command2[i] != '\0'; ++i) {
		RDUARTSendChar(command2[i]);
	}
}

uint8_t GPSGetLocation(double location[], uint8_t *timeH, uint16_t *timeL) {
	
	// NMEA sentence buffer (80 characters)
    char sentence[NMEA_STR_SIZE] = {0};
	
	// Wait for fix
	uint8_t i = 255;
	
	do {
		if (!i--) {
			return -1;
		}
	}
	while (!GPSCheckFix());
	
    GPSGetSentence(sentence);

	GPSParseSentence(sentence,location, timeH, timeL);
	
	return 0;
}

static char GPSCheckFix(void) {
    
    // Check if fix pin is high
    return !((PORTC & (1 << GPS_FIX_PIN)) >> GPS_FIX_PIN);
}

static uint8_t GPSFindCSV(uint8_t value, char sentence[]) {

	uint8_t commas = 0;
	uint8_t i;
    
	for (i = 0; commas < value; i++) {
		commas += sentence[i] == ',';
	}
	
	return i++;
}

static void GPSGetSentence(char sentence[]) {
    
    char startFound = 0;
    char i = 0;
    
    while (!startFound) {
        
		while (!GPSCheckFix());		// Confirm fix
		
        sentence[0] = RDUARTGetChar();
        
		// If start of the sentence ($) found
        if (sentence[0] == '$') {
			
			// Populate buffer
            for (i = 1; i < NMEA_STR_SIZE; ++i) {
                
                sentence[i] = RDUARTGetChar();
                if (sentence[i] == '\n') {
					break;		// Break at end of sentence
				}
            }
			
			// Confirm fix data is valid and GPS still has fix
			if ((sentence[GPSFindCSV(6,sentence)] != '0') && GPSCheckFix()) {
				startFound = 1;
			}
        }
    }
}

static void GPSParseSentence (char sentence[], double location[], uint8_t *timeH, uint16_t *timeL) {
	
	double temp = 0;
	uint8_t offset = GPSFindCSV(1,sentence);
	
	/* For 23:59:59
	 * timeH = 235
	 * timeL = 959
	 */
	
	*timeH = sentence[offset] * 100 + sentence[offset+1] * 10 + sentence[offset+2];
	*timeL = sentence[offset+3] * 100 + sentence[offset+4] * 10 + sentence[offset+5];
	
	/* location[0] : Latitude
	 * location[1] : Longitude
	 * location[2] : Altitude
	 */
	
	// Latitude
	offset = GPSFindCSV(2,sentence);
	
	location[0] = (double)(sentence[offset] - 0x30) * 10 + (double)(sentence[offset+1] - 0x30);
	temp		= (double)(sentence[offset+2] - 0x30) * 10 + (double)(sentence[offset+3] - 0x30) +
				  (double)(sentence[offset+5] - 0x30) * 0.1 + (double)(sentence[offset+6] - 0x30) * 0.01 +
				  (double)(sentence[offset+7] - 0x30) * 0.001 + (double)(sentence[offset+8] - 0x30) * 0.0001;
	temp /= 60;
	location[0] += temp; 
	
	if (sentence[offset+10] == 'S') {
		location[0] *= -1;
	}
	
	// Longitude
	offset = GPSFindCSV(4,sentence);
	
	location[1] = (double)(sentence[offset] - 0x30) * 100 + (double)(sentence[offset+1] - 0x30) * 10 +
	(double)(sentence[offset+2] - 0x30);
	temp		= (double)(sentence[offset+4] - 0x30) * 10 + (double)(sentence[offset+5] - 0x30) +
	(double)(sentence[offset+7] - 0x30) * 0.1 + (double)(sentence[offset+8] - 0x30) * 0.01 +
	(double)(sentence[offset+9] - 0x30) * 0.001 + (double)(sentence[offset+10] - 0x30) * 0.0001;
	temp /= 60;
	location[1] += temp;
	
	if (sentence[offset+11] == 'W') {
		location[1] = 360 - location[1];
	}
	
	// Altitude
	// offset to M after mean altitude - 2 places
	offset = GPSFindCSV(10,sentence) - 2;
		
	location[2] = (double)(sentence[offset] - 0x30) * 0.1 + (double)(sentence[offset-2] - 0x30) +
	(double)(sentence[offset-3] - 0x30) * 10 + (double)(sentence[offset-4] - 0x30) * 100 +
	(double)(sentence[offset-5] - 0x30) * 1000 * (sentence[offset-5] != ',') +
	(double)(sentence[offset-6] - 0x30) * 10000 * (sentence[offset-6] != ',') +
	(double)(sentence[offset-7] - 0x30) * 100000 * (sentence[offset-7] != ',');	

}

#endif	// SBGPS_H_
