// AVR includes
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>

// Bolt specific includes
#include "RDAnalog.h"
#include "RDBluetooth.h"
#include "RDLCD.h"
#include "RDASCIIFont.h"
#include "RDConstants.h"
#include "RDPinDefs.h"
#include "RDSPI.h"
#include "RDUART.h"
#include "RDUtil.h" 
#include "RDI2C.h"
#include "RDSD.h"
#include "SBAccel.h"
#include "SBPressure.h"
#include "SBWDT.h"
#include "SBEEPROM.h"
#include "SBTempHumid.h"
#include "SBGeiger.h"
#include "SBGPS.h"

// FSM defines
#define SENSOR_STATE_IDLE		0
#define SENSOR_STATE_BEGIN		7
#define SENSOR_STATE_TEMPHUMID1	1	
#define SENSOR_STATE_TEMPHUMID2	6	
#define SENSOR_STATE_ACCEL		2
#define SENSOR_STATE_GPS		3
#define SENSOR_STATE_PRESSURE	4
#define SENSOR_STATE_MEMORY		5
#define SENSOR_STATE_GEIGER		8

// Function declarations
void init(char* BTName);
uint8_t sensorFSM(uint8_t state);
void storeData(void);

// Main program
int main (void) {
	SBWDTEn(); // Tic
	
	/* Initialise */
	init("Stratosphere Balloon");
	
	// Check for previous WDT crash
	if(SBEEPROMReadWDTCrashFlag()&&!DEBUG_MODE){
		SBData.SDLoc = SBEEPROMReadSDPoint();
		SBData.numSamples = SBEEPROMReadNumSamples();
		SBData.crashCount = SBEEPROMReadCrashCount();
		SBEEPROMWriteWDTCrashFlag(0);
	} 
	
	SBAccelCal();
	
	// Declare variables
	int8_t buttonVal, sensorReadState = SENSOR_STATE_IDLE;
	
	// Request TempHumid data
	_delay_ms(1000);	// DHT22 needs 800ms minimum to boot
	
	SBWDTDis(); // Toc
	
	char tmp[10];
	while (1) {
		SBWDTEn();	// Tic
		
		if (DEBUG_MODE){ // Force WDT reset
			buttonVal = (PINB>>PB4)&1;
			while(!buttonVal) buttonVal = (PINB>>PB4)&1;
		}
		
		
		itoa(SBData.SDLoc, tmp, 10);
		RDLCDPosition(0, 3);
		RDLCDString(tmp);
		
		if (sensorReadFlag){
			sensorReadState = SENSOR_STATE_BEGIN;
			sensorReadFlag = 0;	
		}
		else sensorReadState = sensorFSM(sensorReadState);
		
		SBWDTDis();	// Toc
    }
}

uint8_t sensorFSM(uint8_t state){
	uint8_t tempHumidCheckVal = 0;
	int tmpX, tmpY, tmpZ;
	int16_t tmpTemp;
	uint16_t tmpHumid; 
	
	switch (state){
		case SENSOR_STATE_IDLE:			// Idle state
			return SENSOR_STATE_IDLE;
			
		case SENSOR_STATE_BEGIN:		// Begin state
			if (DEBUG_MODE){
				RDLCDClear();
			}
			return SENSOR_STATE_ACCEL;
			
		case SENSOR_STATE_PRESSURE:		// Get pressure and altitude data
			if (DEBUG_MODE) {
				SBPressureToLCD();
			}
			return SENSOR_STATE_GPS;
			
		case SENSOR_STATE_GPS:			// Get GPS data
			GPSGetLocation((double*) SBData.location, (uint8_t*) &SBData.timeH, (uint16_t*) &SBData.timeL);
			return SENSOR_STATE_ACCEL;
		
		case SENSOR_STATE_ACCEL:		// Get accelerometer data
			if (DEBUG_MODE) {
				SBAccelToLCD();
			}
			SBAccelGetAccelerationInt(&tmpX, &tmpY, &tmpZ);
			SBData.accelX = tmpX;
			SBData.accelY = tmpY;
			SBData.accelZ = tmpZ;
			return SENSOR_STATE_GEIGER;
			
		case SENSOR_STATE_GEIGER:		// Get Geiger Counter data
			if (DEBUG_MODE) {
				SBGeigerToLCD();
			}
			SBData.cpm = SBGeigerRead();
			return SENSOR_STATE_TEMPHUMID1;
			
		case SENSOR_STATE_TEMPHUMID1:	// Initialise DHT22 data read
			SBTempHumidReqData();
			return SENSOR_STATE_TEMPHUMID2;
			
		case SENSOR_STATE_TEMPHUMID2:	// Read 1-wire communication with DHT22
			tempHumidCheckVal = SBTempHumidCaseCheck();
			if (tempHumidCheckVal == 1){
				SBTempHumidGetVals(&tmpTemp, &tmpHumid);
				SBData.temperature = tmpTemp;
				SBData.humidity = tmpHumid;
				if (DEBUG_MODE)	{
					SBTempHumidDispLCD();
				}
				return SENSOR_STATE_MEMORY;		// Data transfer complete - read next sensor
			} 
			else {
				return SENSOR_STATE_TEMPHUMID2;	// Keep checking TempHumid
			}
			
		case SENSOR_STATE_MEMORY:		// Write data to SD card and location pointer to EEPROM
			storeData();
			return SENSOR_STATE_IDLE;
	}
	return 0;
}


#define SD_BLOCK_SIZE 512
#define PACKET_SIZE_BYTES 36
#define SAMPLE_WRITE_MIN (SD_BLOCK_SIZE - PACKET_SIZE_BYTES - 10)
static uint8_t SDBuffer[512];
static uint16_t bufLen = 0;
void storeData(void){
	memcpy(&SDBuffer[bufLen], (int16_t*)&SBData.numSamples, sizeof(SBData.numSamples));
	bufLen += sizeof(SBData.numSamples);
	memcpy(&SDBuffer[bufLen], (int16_t*)&SBData.crashCount, sizeof(SBData.crashCount));
	bufLen += sizeof(SBData.crashCount);
	memcpy(&SDBuffer[bufLen], (int16_t*)&SBData.temperature, sizeof(SBData.temperature));
	bufLen += sizeof(SBData.temperature);
	memcpy(&SDBuffer[bufLen], (uint16_t*)&SBData.humidity, sizeof(SBData.humidity));
	bufLen += sizeof(SBData.humidity);
	memcpy(&SDBuffer[bufLen], (int*)&SBData.accelX, sizeof(SBData.accelX));
	bufLen += sizeof(SBData.accelX);
	memcpy(&SDBuffer[bufLen], (int*)&SBData.accelY, sizeof(SBData.accelY));
	bufLen += sizeof(SBData.accelY);
	memcpy(&SDBuffer[bufLen], (int*)&SBData.accelZ, sizeof(SBData.accelZ));
	bufLen += sizeof(SBData.accelZ);
	memcpy(&SDBuffer[bufLen], (long*)&SBData.pressure, sizeof(SBData.pressure));
	bufLen += sizeof(SBData.pressure);
	memcpy(&SDBuffer[bufLen], (uint16_t*)&SBData.cpm, sizeof(SBData.cpm));
	bufLen += sizeof(SBData.cpm);
	memcpy(&SDBuffer[bufLen], (double*)&SBData.location[0], sizeof(SBData.location[0]));
	bufLen += sizeof(SBData.location[0]);
	memcpy(&SDBuffer[bufLen], (double*)&SBData.location[1], sizeof(SBData.location[1]));
	bufLen += sizeof(SBData.location[1]);
	memcpy(&SDBuffer[bufLen], (double*)&SBData.location[2], sizeof(SBData.location[2]));
	bufLen += sizeof(SBData.location[2]);
	memcpy(&SDBuffer[bufLen], (uint8_t*)&SBData.timeH, sizeof(SBData.timeH));
	bufLen += sizeof(SBData.timeH);
	memcpy(&SDBuffer[bufLen], (uint16_t*)&SBData.timeL, sizeof(SBData.timeL));
	bufLen += sizeof(SBData.timeL);
	
	SDBuffer[bufLen] = '\n';
	bufLen++;
	SBData.numSamples++;
	
	if (bufLen >= SAMPLE_WRITE_MIN){
		SDBuffer[0] = 0x5E;
		RDSDWriteBuffer(0/*SBData.SDLoc*/, SDBuffer);
		bufLen = 0;
		YLEDPIN |= (1<<YLEDBIT);
		SBData.SDLoc++;
		
		RDLCDClear();
		RDLCDPosition(0, 0);
		for (int i = 0; i < 40; i++) {
			RDLCDCharacter((unsigned char) SDBuffer[i]);
		}
		while(1);
	}
}

void init(char* BTName){
	// set up the system clock
	CLKPR = 0x80;
	CLKPR = 0x00;
	
	// Initialise LEDs
	RLEDDDR |= (1 << RLEDBIT);//led output
	YLEDDDR |= (1 << YLEDBIT);//led output
	RLEDPORT |= (1 << RLEDBIT);// turn on red LED
	
	if (DEBUG_MODE){
		// Initialise LCD
		RDLCDInit();
		RDLCDPosition(0, 0);
		RDLCDClear();
		
		if(BT_ENABLE_MODE){ 
			// Initialise Bluetooth
			RDBluetoothInit(); // init BT
			RDBluetoothConfig(BTName, "1234", '4'); // rename BT
		}
	}
	
	// Initialise ADC
	DDRF &= ~(1<<PF0); // Enable battery monitor ADC pin as input
	RDAnalogInit(0);	// init adcDDRB |= (1<<PB0);
	
	// Set up accelerometer pins
	SBAccelPinsInit();
	
	// Initialise control timers
	SBCtrlInit();
	
	// Initialise I2C
	RDI2CInit(0);
	#define RDI2C_DYNAMIC 1
	
	// Initialise Pressure sensor
	_delay_ms(100);
	SBPressureInit();
	
	// Initialise Geiger Counter
	SBGeigerInit();
	
	// Initialise Temperature and Humidity sensor
	SBTempHumidInit();
	
	// Initialise SD Card
	while(RDSDInit() != 0);
	
	// Initialise GPS
	//GPSInit();
	
	// Manual WDT force reset pins
	if (DEBUG_MODE){
		DDRB &= ~(1<<PB4); // Enable PB4 as input
		PORTB |= (1<<PB4); // Enable Pull-up on PB4
	}
		
	// Turn off init red LED
	RLEDPORT &= ~(1 << RLEDBIT);
}
