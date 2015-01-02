// AVR includes
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>

// Bolt specific includes
#include "EESS_Logo.h"
#include "RDAnalog.h"
#include "RDBluetooth.h"
#include "RDMotor.h"
#include "RDLCD.h"
#include "RDASCIIFont.h"
#include "RDConstants.h"
#include "RDPinDefs.h"
#include "RDSPI.h"
#include "RDUART.h"
#include "RDUtil.h" 
#include "SBAccel.h"
#include "SBPressure.h"
#include "SBTempHumid.h"
#include "SBWDT.h"
#include "SBEEPROM.h"

// Function declarations
void init(char* BTName, char enableBT);

// Defines
#define YLEDPORT PORTC
#define YLEDPIN PINC
#define YLEDBIT PC2
#define YLEDDDR DDRC
#define RLEDPORT PORTD
#define RLEDPIN PIND
#define RLEDBIT PD4
#define RLEDDDR DDRD

// Main program
int main (void) 
{
	SBWDTEn(); // Enable WDT for infinite init loop
	// Initialise
	char BTState = 0;
	init("Stratosphere Balloon", BTState);
	
	// Check for WDT crash
	if(SBEEPROMReadWDTCrashFlag()){
		SDWriteLocPoint = SBEEPROMReadSDPoint();
		SBEEPROMWriteWDTCrashFlag(0);
	}

	// Declare variables
	uint16_t batteryVoltage, i;
	char battVoltStr[10], buttonVal;
	SBWDTDis();
	
	while (1) {
		SBWDTEn();
		for (i = 0; i < 10; i++){
			YLEDPIN |= (1<<YLEDBIT);
			_delay_ms(100);
		}	
		
		buttonVal = (PINB>>PB4)&1;
		while(!buttonVal) buttonVal = (PINB>>PB4)&1;
		
		batteryVoltage = RDAnalogReadBattV();	// Read battery voltage
		itoa(batteryVoltage, battVoltStr, 10);	// Convert battery voltage numerical to sweet, sweet string goodness
		RDLCDPosition(0, 5);					// Work it down low
		RDLCDString((unsigned char*) "BattV:");	// Better formatting than LaTeX
		RDLCDString((unsigned char*) battVoltStr);
		RDLCDString((unsigned char*) "mV");
		
		RLEDPIN |= (1<<RLEDBIT);
		SBWDTDis();
    }
}

// Initialise the Bolt
void init(char* BTName, char enableBT){
	// set up the system clock
	CLKPR = 0x80;
	CLKPR = 0x00;
	
	// Initialise LEDs
	RLEDDDR |= (1 << RLEDBIT);//led output
	YLEDDDR |= (1 << YLEDBIT);//led output
	RLEDPORT |= (1 << RLEDBIT);// turn on red LED
	
	// Initialise ADC
	DDRF &= ~(1<<PF0); // Enable battery monitor ADC pin as input
	RDAnalogInit(0);	// init adcDDRB |= (1<<PB0);
	
	// Initialise LCD
	DDRB |= (1<<PB0);//Permanently enable LCD as selected chip
	PORTB |= (1<<PB0);//Permanently enable LCD as selected chip
	RDLCDInit();
	RDLCDPosition(0, 0);
	RDLCDClear();
	
	// Initialise Bluetooth
	if (enableBT){
		RDBluetoothInit(); // init BT
		RDBluetoothConfig(BTName, "1234", '4'); // rename BT
	}
	
	// Set up accelerometer pins
	SBAccelPinsInit();
	
	// Manual battery check disable pin
	DDRB &= ~(1<<PB4); // Enable PB4 as input
	PORTB |= (1<<PB4); // Enable Pull-up on PB4
	
	// Turn off init red LED
	RLEDPORT &= ~(1 << RLEDBIT);
}
