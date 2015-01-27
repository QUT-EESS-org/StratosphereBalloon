/*
 * Stratosphere Balloon
 * SBCtrl.h
 * Purpose: Controls general system functionality such as timing
 * Created: 30/12/2014
 * Author(s): Blake Fuller
 * Status: UNTESTED
 */ 
 
#ifndef SBCTRL_H_
#define SBCTRL_H_

// Helper defines
#define YLEDPORT PORTC
#define YLEDPIN PINC
#define YLEDBIT PC2
#define YLEDDDR DDRC
#define RLEDPORT PORTD
#define RLEDPIN PIND
#define RLEDBIT PD4
#define RLEDDDR DDRD

#define DEBUG_MODE 1
#define BT_ENABLE_MODE 0
#define ASCII_SMALL_FONT 1
#define ASCII_LARGE_FONT 0

// Inline defines
#define TIMER3_OFF	TCCR3B &= ~(1<<CS30); DHT22Counter = 0; TCNT3 = 0
#define TIMER3_ON 	TCCR3B |= (1<<CS30)

volatile uint32_t DHT22Counter = 0;
volatile uint8_t sensorReadFlag = 0, flagCount = 0;

// Sensor data struct
typedef struct{
	double temperature;		// See SBTempHumid
	double humidity;		// See SBTempHumid
	int accelX;				// See SBAccel
	int accelY;				// See SBAccel
	int accelZ;				// See SBAccel
	long pressure;			// See SBPressure
	uint16_t cpm;			// See SBGeiger
	double location[3];		// See SBGPS
	uint8_t timeH;			// See SBGPS
	uint16_t timeL;			// See SBGPS
	uint32_t crashCount;	// See SBEEPROM
	uint32_t numSamples;	// See SBEEPROM
	uint32_t SDLoc;			// See SBEEPROM
} SBDataStruct;

volatile SBDataStruct SBData;
 
void SBCtrlInit(void){
	TCCR1A = 0;
	TCCR1B = 	(1<<WGM12)|	// Set up Timer/Counter 1 in CTC mode
				(1<<CS12);	// Prescaler of 256
	TCNT1 = 0;			
	OCR1A = 0x7A12;			// Set up general control interrupt for 500ms
	TIMSK1 = (1<<OCIE1A);	// enable timer
	
	TCCR3A = 0;
	TCCR3B = 	(1<<WGM32)|	// Set up Timer/Counter 3 in CTC mode
				(1<<CS30);	// Prescaler of 1
	TCNT3 = 0;
	OCR3A = 0x03C0;			// Set up 1-wire trigger counter for 60us 
	TIMSK3 = (1<<OCIE3A);	// enable timer
}

ISR(TIMER1_COMPA_vect){
	flagCount++;
	if (flagCount == 2){
		sensorReadFlag = 1;
		flagCount = 0;
	}
}

ISR(TIMER3_COMPA_vect){
	DHT22Counter++;
}

#endif