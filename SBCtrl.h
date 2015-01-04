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

// Inline defines
#define TIMER3_OFF	TCCR3B &= ~(1<<CS30); DHT22Counter = 0; TCNT3 = 0
#define TIMER3_ON 	TCCR3B |= (1<<CS30)

volatile uint32_t DHT22Counter = 0;
volatile uint8_t sensorReadFlag = 0;
 
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
	sensorReadFlag = 1;
	RLEDPIN |= (1<<RLEDBIT);
}

ISR(TIMER3_COMPA_vect){
	DHT22Counter++;
}

#endif