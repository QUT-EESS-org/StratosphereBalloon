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

// LED defines
#define YLEDPORT PORTC
#define YLEDPIN PINC
#define YLEDBIT PC2
#define YLEDDDR DDRC
#define RLEDPORT PORTD
#define RLEDPIN PIND
#define RLEDBIT PD4
#define RLEDDDR DDRD

volatile uint32_t DHT22Counter = 0;
 
void SBCtrlInit(void){
	TCCR1A = 0;
	TCCR1B = 	(1<<WGM12)|	// Set up Timer/Counter 1 in CTC mode
				(1<<CS10);	// Prescaler of 1
	TCNT1 = 0xF9C0;			// Set up base TCNT
	OCR1A = 0x063F;			// Set up general control interrupt for 100us
	//TIMSK1 = (1<<OCIE1A);	// enable timer
	
	TCCR3A = 0;
	TCCR3B = 	(1<<WGM32)|	// Set up Timer/Counter 3 in CTC mode
				(1<<CS30);	// Prescaler of 1
	TCNT3 = 0;
	OCR3A = 0x00A0;			// Set up general control interrupt for 10us 
	TIMSK3 = (1<<OCIE3A);	// enable timer
}

ISR(TIMER1_COMPA_vect){
	
}

ISR(TIMER3_COMPA_vect){
	DHT22Counter++;
}

#endif