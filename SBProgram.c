// AVR includes
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>

// Bolt specific includes
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
#include "SBWDT.h"
#include "SBEEPROM.h"
#include "SBTempHumid.h"

// FSM defines
#define SENSOR_STATE_IDLE		0
#define SENSOR_STATE_BEGIN		7
#define SENSOR_STATE_TEMPHUMID1	1	
#define SENSOR_STATE_TEMPHUMID2	6	
#define SENSOR_STATE_ACCEL		2
#define SENSOR_STATE_GPS		3
#define SENSOR_STATE_PRESSURE	4
#define SENSOR_STATE_MEMORY		5

// Sensor data struct
typedef struct{
	double temperature;		// See SBTempHumid
	double humidity;		// See SBTempHumid
	int accelX;				// See SBAccel
	int accelY;				// See SBAccel
	int accelZ;				// See SBAccel
} SBDataStruct;

static SBDataStruct SBData;

// Function declarations
void init(char* BTName);
uint8_t sensorFSM(uint8_t state);

// Main program
int main (void) 
{
	SBWDTEn(); // Tic
	
	/* Initialise */
	init("Stratosphere Balloon");
	SBAccelCal();
	_delay_ms(2000);
	// Check for previous WDT crash
	if(SBEEPROMReadWDTCrashFlag()){
		SDWriteLocPoint = SBEEPROMReadSDPoint();
		SBEEPROMWriteWDTCrashFlag(0);
	}

	// Declare variables
	int8_t buttonVal, sensorReadState = SENSOR_STATE_IDLE;
	
	// Request TempHumid data
	_delay_ms(1000);	// DHT22 needs 800ms minimum to boot
	
	SBWDTDis(); // Toc
	
	while (1) {
		SBWDTEn();	// Tic
		
		if (DEBUG_MODE){ // Force WDT reset
			buttonVal = (PINB>>PB4)&1;
			while(!buttonVal) buttonVal = (PINB>>PB4)&1;
		}
		
		if (sensorReadFlag){
			sensorReadState = SENSOR_STATE_BEGIN;
			sensorReadFlag = 0;	
		}else sensorReadState = sensorFSM(sensorReadState);
		
		SBWDTDis();	// Toc
    }
}

uint8_t sensorFSM(uint8_t state){
	uint8_t tempHumidCheckVal = 0;
	
	switch (state){
		case SENSOR_STATE_IDLE:			// Idle state
			return SENSOR_STATE_IDLE;
			
		case SENSOR_STATE_BEGIN:		// Begin state
			return SENSOR_STATE_ACCEL;
		
		case SENSOR_STATE_ACCEL:		// Get accelerometer data
			if (DEBUG_MODE) SBAccelToLCD();
			SBAccelGetAccelerationInt(&SBData.accelX, &SBData.accelY, &SBData.accelY);
			return SENSOR_STATE_GPS;
			
		case SENSOR_STATE_GPS:			// Get GPS data
			return SENSOR_STATE_PRESSURE;
			
		case SENSOR_STATE_PRESSURE:		// Get pressure and altitude data
			return SENSOR_STATE_TEMPHUMID1;
			
		case SENSOR_STATE_TEMPHUMID1:	// Initialise DHT22 data read
			SBTempHumidReqData();
			return SENSOR_STATE_TEMPHUMID2;
			
		case SENSOR_STATE_TEMPHUMID2:	// Read 1-wire communication with DHT22
			tempHumidCheckVal = SBTempHumidCaseCheck();
			if (tempHumidCheckVal == 1){
				SBTempHumidGetVals(&SBData.temperature, &SBData.humidity);
				if (DEBUG_MODE)	SBTempHumidDispLCD();
				return SENSOR_STATE_MEMORY;		// Data transfer complete - read next sensor
			} else return SENSOR_STATE_TEMPHUMID2;	// Keep checking TempHumid
			
		case SENSOR_STATE_MEMORY:		// Write data to SD card and location pointer to EEPROM
			return SENSOR_STATE_IDLE;
	}
	return 0;
}

void init(char* BTName){
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
	
	if (DEBUG_MODE){
		// Initialise LCD
		DDRB |= (1<<PB0);//Permanently enable LCD as selected chip
		PORTB |= (1<<PB0);//Permanently enable LCD as selected chip
		RDLCDInit();
		RDLCDPosition(0, 0);
		RDLCDClear();
		
		if(BT_ENABLE_MODE){ 
			// Initialise Bluetooth
			RDBluetoothInit(); // init BT
			RDBluetoothConfig(BTName, "1234", '4'); // rename BT
		}
	}
	
	// Set up accelerometer pins
	SBAccelPinsInit();
	
	// Initialise control timers
	SBCtrlInit();
	
	// Initialise Temperature and Humidity sensor
	SBTempHumidInit();
	
	// Manual WDT force reset pins
	if (DEBUG_MODE){
		DDRB &= ~(1<<PB4); // Enable PB4 as input
		PORTB |= (1<<PB4); // Enable Pull-up on PB4
	}
		
	// Turn off init red LED
	RLEDPORT &= ~(1 << RLEDBIT);
}
