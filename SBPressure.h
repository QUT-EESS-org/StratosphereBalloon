/*
 * Stratosphere Balloon
 * SBPressure.h
 * Purpose: Abstracts all Pressure sensor functions
 * Created: 30/12/2014
 * Author(s): Blake Fuller, Shaun Karran
 * Status: UNTESTED
 */ 

#include <util/delay.h>
//#include "RDI2C.h" // Didnt work. Switched to example i2c code.
#include "i2c.h"

#define BMP085_WRITE 	0xEE
#define BMP085_READ 	0xEF

#define BMP085_AC1 	0xAA
#define BMP085_AC2 	0xAC
#define BMP085_AC3 	0xAE
#define BMP085_AC4 	0xB0
#define BMP085_AC5 	0xB2
#define BMP085_AC6 	0xB4
#define BMP085_B1 	0xB6
#define BMP085_B2 	0xB8
#define BMP085_MB 	0xBA
#define BMP085_MC 	0xBC
#define BMP085_MD 	0xBE

#define BMP085_ADDR 	0xF4
#define BMP085_RESULT 	0xF6

#define BMP085_TEMP 	0x2E
#define BMP085_PRES_OS0 0x34

#define OSS 0	// Oversampling Setting (note: code is not set up to use other OSS values)

typedef struct BMP085_Calib_Data
{
	int16_t ac1;
	int16_t ac2; 
	int16_t ac3; 
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t b1; 
	int16_t b2;
	int16_t mb;
	int16_t mc;
	int16_t md;
} BMP085_Calib_Data;

static BMP085_Calib_Data BMP085CD;

uint16_t BMP085_Read_Reg(uint8_t addr)
{
	// uint8_t addrBuf[1];
	// uint8_t readBuff[2];
	// addrBuf[0] = addr;

	// // !! Never gets past either of these functions !!
	// RDI2CWrite(BMP085_WRITE, addrBuf, 1);
	// RDI2CRead(BMP085_READ, readBuff, 2);

	// return ((readBuff[0] << 8) | readBuff[1]);

	uint8_t msb, lsb;

	i2cSendStart();
	i2cWaitForComplete();
	
	i2cSendByte(BMP085_WRITE);	// write 0xEE
	i2cWaitForComplete();
	
	i2cSendByte(addr);	// write register address
	i2cWaitForComplete();
	
	i2cSendStart();
	
	i2cSendByte(BMP085_READ);	// write 0xEF
	i2cWaitForComplete();
	
	i2cReceiveByte(TRUE);
	i2cWaitForComplete();
	msb = i2cGetReceivedByte();	// Get MSB result
	i2cWaitForComplete();
	
	i2cReceiveByte(FALSE);
	i2cWaitForComplete();
	lsb = i2cGetReceivedByte();	// Get LSB result
	i2cWaitForComplete();
	
	i2cSendStop();
	
	return ((msb << 8) | lsb);
}

int32_t BMP085_Read_Temp(void)
{
	// uint8_t addrBuf[2];
	// addrBuf[0] = BMP085_ADDR;
	// addrBuf[1] = BMP085_TEMP;

	// RDI2CWrite(BMP085_WRITE, addrBuf, 2);

	i2cSendStart();
	i2cWaitForComplete();
	
	i2cSendByte(BMP085_WRITE);	// write 0xEE
	i2cWaitForComplete();
	
	i2cSendByte(BMP085_ADDR);	// write register address
	i2cWaitForComplete();
	
	i2cSendByte(BMP085_TEMP);	// write register data for temp
	i2cWaitForComplete();
	
	i2cSendStop();

	_delay_ms(10);

	return BMP085_Read_Reg(BMP085_RESULT);
}

int32_t BMP085_Read_Pressure(void)
{
	// uint8_t addrBuf[2];
	// addrBuf[0] = BMP085_ADDR;
	// addrBuf[1] = BMP085_PRES_OS0;

	// RDI2CWrite(BMP085_WRITE, addrBuf, 2);

	i2cSendStart();
	i2cWaitForComplete();
	
	i2cSendByte(BMP085_WRITE);	// write 0xEE
	i2cWaitForComplete();
	
	i2cSendByte(BMP085_ADDR);	// write register address
	i2cWaitForComplete();
	
	i2cSendByte(BMP085_PRES_OS0);	// write register data for pressure
	i2cWaitForComplete();
	
	i2cSendStop();

	_delay_ms(10);

	return BMP085_Read_Reg(BMP085_RESULT);
}

void BMP085_Read_Calibration_Data(void)
{
	i2cInit();

	BMP085CD.ac1 = BMP085_Read_Reg(BMP085_AC1);
	BMP085CD.ac2 = BMP085_Read_Reg(BMP085_AC2);
	BMP085CD.ac3 = BMP085_Read_Reg(BMP085_AC3);
	BMP085CD.ac4 = BMP085_Read_Reg(BMP085_AC4);
	BMP085CD.ac5 = BMP085_Read_Reg(BMP085_AC5);
	BMP085CD.ac6 = BMP085_Read_Reg(BMP085_AC6);
	BMP085CD.b1 = BMP085_Read_Reg(BMP085_B1);
	BMP085CD.b2 = BMP085_Read_Reg(BMP085_B2);
	BMP085CD.mb = BMP085_Read_Reg(BMP085_MB);
	BMP085CD.mc = BMP085_Read_Reg(BMP085_MC);
	BMP085CD.md = BMP085_Read_Reg(BMP085_MD);
}

void BMP085_Convert(int32_t* temp, int32_t* pressure)
{
	int32_t ut;
	int32_t up;
	int32_t x1, x2, b5, b6, x3, b3, p;
	uint32_t b4, b7;

	ut = BMP085_Read_Temp();
	up = BMP085_Read_Pressure();

	// Calculate tempurature.
	x1 = (ut - BMP085CD.ac6) * BMP085CD.ac5 >> 15;
	x2 = ((int32_t)BMP085CD.mc << 11) / (x1 + BMP085CD.md);
	b5 = x1 + x2;
	*temp = (b5 + 8) >> 4;
	
	// Calculate pressure.
	b6 = b5 - 4000;
	x1 = (BMP085CD.b2 * (b6 * b6 >> 12)) >> 11;
	x2 = BMP085CD.ac2 * b6 >> 11;
	x3 = x1 + x2;
	b3 = (((int32_t)BMP085CD.ac1 * 4 + x3) + 2)/4;
	x1 = BMP085CD.ac3 * b6 >> 13;
	x2 = (BMP085CD.b1 * (b6 * b6 >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (BMP085CD.ac4 * (uint32_t)(x3 + 32768)) >> 15;
	b7 = ((uint32_t)up - b3) * (50000 >> OSS);
	p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	*pressure = p + ((x1 + x2 + 3791) >> 4);
}

int32_t BMP085_Altitude(int32_t pressure)
{
	int32_t altitude;
	double temp;

	temp = (double)pressure / 101325; // 101325Pa is standard pressure at sea level.
	temp = 1-pow(temp, 0.19029);
	altitude = round(44330*temp);

	return altitude;
}
