#include "pres/bmp085.h"

void SBPressureInit(void){
	BMP085_Calibration();
}

void SBCalculateAltitude(long* altitude, long pressure){
	double temporary;

	temporary = (double)pressure / 101325; // 101325Pa is std pressure at sea level.
	temporary = 1-pow(temp, 0.19029);
	*altitude = round(44330 * temporary);
}

void SBPressureGetData(long *pressure, long *temperature, long *altitude){
	bmp085Convert(temperature, pressure);
	SBCalculateAltitude(altitude, *pressure);
}

void SBPressureToLCD(void){
	long pres, temp, alt;
	SBPressureGetData(&pres, &temp, &alt);
	
	unsigned char tempStr[11], presStr[11], altStr[11];
	itoa(pres, presStr, 10);
	itoa(temp, tempStr, 10);
	itoa(alt, altStr, 10);
	
	RDLCDPosition(0, 3);					
	RDLCDString((unsigned char*) "Temp:     ");
	RDLCDPosition(42, 3);
	RDLCDString(tempStr);
	RDLCDString((unsigned char*) "C");
	RDLCDPosition(0, 4);					
	RDLCDString((unsigned char*) "Pres:     ");
	RDLCDPosition(42, 4);
	RDLCDString(presStr);
	RDLCDString((unsigned char*) "Pa");
	RDLCDPosition(0, 5);					
	RDLCDString((unsigned char*) "Alt:     ");
	RDLCDPosition(42, 5);
	RDLCDString(altStr);
	RDLCDString((unsigned char*) "m");
}