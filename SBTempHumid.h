/*
 * Stratosphere Balloon
 * SBTempHumid.h
 * Purpose: Abstracts all Temperature and Humidity (DHT22) functions
 * Created: 30/12/2014
 * Author(s): Blake Fuller
 * Status: UNTESTED
 */ 
 
#ifndef SBTEMPHUMID_H_
#define SBTEMPHUMID_H_

#include <avr/io.h>
#include "SBCtrl.h"

#define INT6_LOW_LEVEL								// Logical low trigger
#define INT6_ANY_LOG_CHANGE	(1<<ISC60)				// Any logical change trigger
#define INT6_FALL_EDGE		(1<<ISC61)				// Falling edge trigger
#define INT6_RISE_EDGE		(1<<ISC61)|(1<<ISC60)	// Rising edge trigger

#define SBTH_SDA_PIN	PINE
#define SBTH_SDA_PORT	PORTE
#define SBTH_SDA_DDR	DDRE
#define SBTH_SDA_BIT	PE6

#define SBTH_STATE_IDLE			0
#define SBTH_STATE_REQ_DATA		1
#define SBTH_STATE_PULL_HIGH	2
#define SBTH_STATE_ACK_LOW		3
#define SBTH_STATE_ACK_HIGH		4
#define SBTH_STATE_DATA_LOW		5
#define SBTH_STATE_DATA			6
#define SBTH_STATE_FINALISE		7

#define SBTH_REQ_DELAY			100		// Data request delay is 1000us
#define SBTH_DATA_LOGIC_VALUE	5		// If state is high for >50us then "1" else "0"
#define SBTH_NUM_DATA_BITS		4		// 40 bits in a data transfer

volatile static uint8_t oneWireCommState = SBTH_STATE_IDLE;
volatile static uint8_t oneWireDataBitNum = 0, pinState;
volatile static uint64_t dataBuffer1, dataBuffer2;
static int16_t _temperature;
static uint16_t _humidity;
 
void SBTempHumidInit(void){
	SBTH_SDA_DDR &= ~(1<<SBTH_SDA_BIT);	// PE7-INT6 is used as the hardware interrupt pin
	SBTH_SDA_PORT &= ~(1<<SBTH_SDA_BIT);	// Disable pull-up - handled by external hardware
	
	/* NOTE: INT0:INT3 use EICRA, INT4:INT7 use EICRB */
	
	EICRB |= INT6_ANY_LOG_CHANGE;	// Set up INT6 as any logical change interrupt
	EIMSK |= (1<<INT6);				// Enable INT6
}

void SBTempHumidReqData(){
	if (oneWireCommState == SBTH_STATE_IDLE){
		oneWireCommState = SBTH_STATE_REQ_DATA;
		DDRE |= (1<<PE6);	// req data
		PORTE |= (1<<PE6);	// req data
		DHT22Counter = 0;
	}
}

int8_t SBTempHumidCaseCheck(void){	
	unsigned char stateStr[10], countStr[10];
	itoa(oneWireCommState, stateStr, 10);
	itoa(DHT22Counter, countStr, 10);
	
	RDLCDPosition(0, 2);
	RDLCDString(stateStr);
	RDLCDPosition(0, 3);
	RDLCDString("       ");
	RDLCDPosition(0, 3);
	RDLCDString(countStr);
	
	switch (oneWireCommState){
		case SBTH_STATE_IDLE:
			return -2; // Flag IDLE state
			
		case SBTH_STATE_REQ_DATA:
			if (DHT22Counter >= SBTH_REQ_DELAY){
				dataBuffer2 = dataBuffer1;
				dataBuffer1 = 0;
				oneWireDataBitNum = 0;
				SBTH_SDA_DDR &= ~(1<<SBTH_SDA_BIT);		// Set SDA as input
				SBTH_SDA_PORT &= ~(1<<SBTH_SDA_BIT);	// Disable pull-up - handled by external hardware
				oneWireCommState = SBTH_STATE_PULL_HIGH;// Change state
			}
			return 0; 
			
		case SBTH_STATE_PULL_HIGH:
			return 0; // Do nothing
			
		case SBTH_STATE_ACK_LOW:
			return 0; // Do nothing
			
		case SBTH_STATE_ACK_HIGH:
			return 0; // Do nothing
			
		case SBTH_STATE_DATA_LOW:
			return 0; // Do nothing
		
		case SBTH_STATE_DATA:
			return 0; // Do nothing
		
		case SBTH_STATE_FINALISE:{
			uint8_t parityRx, parity, temperature1, temperature2, humidity1, humidity2;
			parityRx 		= dataBuffer1&0x00000000FF;
			temperature1 	= dataBuffer1&0x000000FF00;
			temperature2 	= dataBuffer1&0x0000FF0000;
			humidity1 		= dataBuffer1&0x00FF000000;
			humidity2 		= dataBuffer1&0xFF00000000;
			// Calculate parity. Note that overflows are expected and allowed
			parity = temperature1 + temperature2 + humidity1 + humidity2; 
			oneWireCommState = SBTH_STATE_IDLE;
			if (parity == parityRx){
				_temperature = (temperature2<<8)|(temperature1);
				_humidity = (humidity2<<8)|(humidity1);
				return 1;
			} else return -1;
		}
	}
}

void SBTempHumidGetVals(int16_t* temp, uint16_t* humid){
	*temp = _temperature;
	*humid = _humidity;
}

void SBTempHumidDispLCD(int16_t temp, uint16_t humid){
	unsigned char tempStr[10], humidStr[10];
	itoa(temp, tempStr, 10);
	itoa(humid, humidStr, 10);
	
	RDLCDPosition(0, 0);					
	RDLCDString((unsigned char*) "Temp:     ");
	RDLCDPosition(35, 0);
	RDLCDString(tempStr);
	RDLCDString((unsigned char*) "C");
	RDLCDPosition(0, 1);					
	RDLCDString((unsigned char*) "Humid:     ");
	RDLCDPosition(35, 1);
	RDLCDString(humidStr);
	RDLCDString((unsigned char*) "%");
}

void SBTempHumidEnInt(void){
	EIMSK |= (1<<INT6);		// Enable INT6
}

void SBTempHumidDisInt(void){
	EIMSK &= ~(1<<INT6);	// Disable INT6
}

ISR(INT6_vect){
	switch (oneWireCommState){
		case SBTH_STATE_IDLE:
			break; // Do nothing
			
		case SBTH_STATE_REQ_DATA:
			break; // Do nothing
			
		case SBTH_STATE_PULL_HIGH:	// Wait for DHT22 to ack low
			pinState = (SBTH_SDA_PIN>>SBTH_SDA_BIT)&1;	// Read SDA
			if (!pinState) oneWireCommState = SBTH_STATE_ACK_LOW;
			break;
			
		case SBTH_STATE_ACK_LOW:	// Wait for DHT22 to ack high
			pinState = (SBTH_SDA_PIN>>SBTH_SDA_BIT)&1;	// Read SDA
			if (pinState) oneWireCommState = SBTH_STATE_ACK_HIGH;
			break;
			
		case SBTH_STATE_ACK_HIGH:	// Wait for data transmission
			pinState = (SBTH_SDA_PIN>>SBTH_SDA_BIT)&1;	// Read SDA
			if (!pinState) oneWireCommState = SBTH_STATE_DATA_LOW;
			break;
			
		case SBTH_STATE_DATA_LOW:	// Wait for data transmission
			pinState = (SBTH_SDA_PIN>>SBTH_SDA_BIT)&1;	// Read SDA
			if (pinState){
				oneWireCommState = SBTH_STATE_DATA;
				DHT22Counter = 0;
			}
			break;
		
		case SBTH_STATE_DATA:	// Dictate whether "0" or "1" received 
			pinState = (SBTH_SDA_PIN>>SBTH_SDA_BIT)&1;		// Read SDA
			if (!pinState){
				if (DHT22Counter >= SBTH_DATA_LOGIC_VALUE){	// Analyse received data
					dataBuffer1 = (dataBuffer1<<1)|1;		// Shift "1" in
				} else{
					dataBuffer1 = (dataBuffer1<<1);			// Shift "0" in
				}
				oneWireDataBitNum++;						// Increment bit number
				if (oneWireDataBitNum > SBTH_NUM_DATA_BITS)	// If whole message received
					oneWireCommState = SBTH_STATE_FINALISE;	// Finalise message
				else 
					oneWireCommState = SBTH_STATE_DATA_LOW;	// Read in more data
			}
			break;
		
		case SBTH_STATE_FINALISE:
			break; // Do nothing
	}
}

#endif