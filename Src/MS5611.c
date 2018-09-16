/*
 * MS5611.c
 *
 *  Created on: 16 сент. 2018 г.
 *      Author: denisdenk
 */

#include "MS5611.h"
#include "math.h"
#include "main.h"
#include "i2c.h"

uint16_t ms5611ReadShort(uint8_t address) {

	uint8_t rxBuf[2] = {0};
	HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)MS5611_addr << 1, &address, 1, 0x100);
	HAL_I2C_Master_Receive(&hi2c1, (uint16_t)MS5611_addr << 1, rxBuf, 2, 100);
	return (rxBuf[0] << 8) | rxBuf[1];
}


uint32_t ms5611ReadLong(uint8_t address) {

	uint8_t rxBuf[3] = {0};
	HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)MS5611_addr << 1, &address, 1, 0x100);
	HAL_I2C_Master_Receive(&hi2c1, (uint16_t)MS5611_addr << 1, rxBuf, 3, 100);
	return (rxBuf[0] << 16) | (rxBuf[1] << 8) | rxBuf[2];
}


void ms5611WriteByte(uint8_t data) {

	HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)MS5611_addr << 1, &data, 1, 0x100);
}
//----------------------------------------

void MS5611_Init(void) {
	// Reset
	ms5611WriteByte(0x1E);

	HAL_Delay(5);

	// Read calibration data
	ms5611_C1 = ms5611ReadShort(0xA2);
	ms5611_C2 = ms5611ReadShort(0xA4);
	ms5611_C3 = ms5611ReadShort(0xA6);
	ms5611_C4 = ms5611ReadShort(0xA8);
	ms5611_C5 = ms5611ReadShort(0xAA);
	ms5611_C6 = ms5611ReadShort(0xAC);
}

void MS5611_Convert(long* temperature, long* pressure) {

	unsigned long D1, D2;
	long dT, TEMP;
	long long OFF, SENS, OFF2, SENS2, T2;

	// Start Pressure conversion
	ms5611WriteByte(0x40 + MS5611_OSR * 2);
	HAL_Delay(1 + MS5611_OSR * 2);
	// Read Pressure data
	D1 = ms5611ReadLong(0x00);

	// Start Temperature conversion
	ms5611WriteByte(0x50 + MS5611_OSR * 2);
	HAL_Delay(1 + MS5611_OSR * 2);
	// Read Temperature data
	D2 = ms5611ReadLong(0x00);

	dT = D2 - (ms5611_C5 << 8);
	TEMP = 2000 + (((long long)dT * (long long)ms5611_C6) >> 23);
	OFF = ((long long)ms5611_C2 << 16) + (((long long)ms5611_C4 * (long long)dT) >> 7);
	SENS = ((long long)ms5611_C1 << 15 ) + (((long long)ms5611_C3 * (long long)dT ) >> 8);

	if (TEMP >= 2000) {
		T2 = 0;
		OFF2 = 0;
		SENS2 = 0;
	}
	else if (TEMP < 2000) {
		T2 = (((long long)dT * (long long)dT) >> 31);
		OFF2 = 5 * (((long long)TEMP - 2000) * ((long long)TEMP - 2000)) >> 1;
		SENS2 = 5 * (((long long)TEMP - 2000) * ((long long)TEMP - 2000)) >> 2;
		if (TEMP < -1500 ) {
			OFF2 = OFF2 + 7 * (((long long)TEMP + 1500) * ((long long)TEMP + 1500));
			SENS2 = SENS2 + ((11 *(((long long)TEMP + 1500) * ((long long)TEMP + 1500))) >> 1);
		}
	}

	TEMP = TEMP - T2;
	OFF = OFF - OFF2;
	SENS = SENS - SENS2;

	*pressure = (unsigned long) (((((D1 * SENS) >> 21) - OFF)) >> 15);
	*temperature = (long)TEMP/100;
}
