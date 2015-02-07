#include "pres/bmp085.h"

void SBPressureInit(void){
	BMP085_Calibration();
}

void SBPressureGetData(long *pressure, long *temperature){
	bmp085Convert(temperature, pressure);
}

void SBPressureToLCD(void){
	long pres, temp;
	SBPressureGetData(&pres, &temp);
	
	unsigned char tempStr[5], presStr[5];
	itoa(pres, presStr, 10);
	itoa(temp, tempStr, 10);
	
	RDLCDPosition(0, 3);					
	RDLCDString((unsigned char*) "Temp:     ");
	RDLCDPosition(42, 3);
	RDLCDString(tempStr);
	RDLCDString((unsigned char*) "C");
	RDLCDPosition(0, 4);					
	RDLCDString((unsigned char*) "Pres:     ");
	RDLCDPosition(42, 4);
	RDLCDString(presStr);
	RDLCDString((unsigned char*) "%");
}