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

#define SBTH_REQ_DELAY			17		// Data request delay is 1000us
#define SBTH_NUM_DATA_BITS		39		// 40 bits in a data transfer

volatile static uint8_t oneWireCommState = SBTH_STATE_IDLE;
volatile static uint8_t oneWireDataBitNum = 0, pinState;
volatile static uint64_t dataBuffer;
static double _temperature;
static double _humidity;
 
void SBTempHumidInit(void){
	SBTH_SDA_DDR &= ~(1<<SBTH_SDA_BIT);	// PE7-INT6 is used as the hardware interrupt pin
	SBTH_SDA_PORT &= ~(1<<SBTH_SDA_BIT);	// Disable pull-up - handled by external hardware
	
	/* NOTE: INT0:INT3 use EICRA, INT4:INT7 use EICRB */
	
	EICRB |= INT6_ANY_LOG_CHANGE;	// Set up INT6 as any logical change interrupt
	EIMSK |= (1<<INT6);				// Enable INT6
}

void SBTempHumidReqData(){
	if (oneWireCommState == SBTH_STATE_IDLE){
		TIMSK3 |= (1<<OCIE3A);	// Enable timer
		oneWireCommState = SBTH_STATE_REQ_DATA;
		SBTH_SDA_DDR |= (1<<SBTH_SDA_BIT);	// req data
		SBTH_SDA_PORT &= ~(1<<SBTH_SDA_BIT);	// req data
		DHT22Counter = 0;
		TIMER3_ON;
	}
}

int8_t SBTempHumidCaseCheck(void){
	switch (oneWireCommState){
		case SBTH_STATE_IDLE:
			TIMER3_OFF;
			return -2; // Flag IDLE state
			
		case SBTH_STATE_REQ_DATA:
			if (DHT22Counter >= SBTH_REQ_DELAY){
				dataBuffer = 0;
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
			parityRx 		= dataBuffer&0x00000000FF;
			temperature1 	= (dataBuffer&0x000000FF00)>>8;
			temperature2 	= (dataBuffer&0x0000FF0000)>>16;
			humidity1 		= (dataBuffer&0x00FF000000)>>24;
			humidity2 		= (dataBuffer&0xFF00000000)>>32;
			// Calculate parity. Note that overflows are expected and allowed
			parity = temperature1 + temperature2 + humidity1 + humidity2; 
			oneWireCommState = SBTH_STATE_IDLE;
			RLEDPIN |= (1<<RLEDBIT);
			if (/*parity == parityRx*/1){
				_temperature = (double)((int)((temperature2<<8)|(temperature1)))/10;//dat math
				_humidity = (double)((int)((humidity2<<8)|(humidity1)))/10;//dat math
				return 1;
			} else return -1;
		}
	}
	return 0;
}

void SBTempHumidGetVals(double* temp, double* humid){
	*temp = _temperature;
	*humid = _humidity;
}

void SBTempHumidDispLCD(void){
	unsigned char tempStr[5], humidStr[5];
	
	sprintf(tempStr, "%0.1f", _temperature);
	sprintf(humidStr, "%0.1f", _humidity);
	
	RDLCDPosition(0, 0);					
	RDLCDString((unsigned char*) "Temp:     ");
	RDLCDPosition(42, 0);
	RDLCDString(tempStr);
	RDLCDString((unsigned char*) "C");
	RDLCDPosition(0, 1);					
	RDLCDString((unsigned char*) "Humid:     ");
	RDLCDPosition(42, 1);
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
				TIMER3_ON;
				DHT22Counter = 0;
				oneWireCommState = SBTH_STATE_DATA;
			}
			break;
		
		case SBTH_STATE_DATA:	// Dictate whether "0" or "1" received 
			pinState = (SBTH_SDA_PIN>>SBTH_SDA_BIT)&1;		// Read SDA
			if (!pinState){
				if (DHT22Counter){	// Analyse received data
					dataBuffer = (dataBuffer<<1)|1;		// Shift "1" in
				} else{
					dataBuffer = (dataBuffer<<1);			// Shift "0" in
				}
				oneWireDataBitNum++;						// Increment bit number
				if (oneWireDataBitNum > SBTH_NUM_DATA_BITS)	// If whole message received
					oneWireCommState = SBTH_STATE_FINALISE;	// Finalise message
				else 
					oneWireCommState = SBTH_STATE_DATA_LOW;	// Read in more data
				TIMER3_OFF;
			}
			break;
		
		case SBTH_STATE_FINALISE:
			break; // Do nothing
	}
}

#endif