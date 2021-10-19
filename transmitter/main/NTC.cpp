
/*
 * NTC.c
 *
 *  Created on: Aug 20, 2017
 *      Author: Dig Kleppe
 *
 *      calculates and averages NTC measurements
 *      values are averaged over 10 measurements to give more stable/accurate output.
 *
 */


#include <stdint.h>
#include <math.h>
#include "ntc.h"
#include <stdio.h>

//http://www.giangrandi.ch/electronics/ntc/ntc.shtml

//#define IKEA

#ifdef IKEA
 #define RREF 47000.0  // vishay
 #define R25  220000L
 #define BVALUE 4225  // Ikea
#else
	#define RREF 10000  // vishay
	#define R25	 10000L   /// vishay
	#define BVALUE 3977 //  10k       100k 2322 640 5 Vishay
#endif



int32_t NTCaveragingBuffer[NR_NTCS][NTCAVERAGING + 2];


float temperature[NR_NTCS];
float resistance[NR_NTCS];


float calcTemp(uint32_t ID);
float NTCreferenceValue;


// calculates resistance from ADC value
float calcNTC( uint32_t ADCVal) {
	float ntc;
	ntc = RREF * (float) ADCVal / ( NTCreferenceValue - (float) ADCVal);  // is RREF is connected to 3V3 with separate reference
	return (ntc);
}

// calulates temperature from resistance
float resToTemp(float Rntc) {
	float temp;
	temp = (1.0 / ((log(Rntc / R25)) / BVALUE +(1.0 / 298.15))) - 273.0; //log = ln
	return (temp);
}

static uint8_t tabPntr[NR_NTCS];


// buffers and avarages ADC Values and calculates according temperatures.

float updateNTC(uint32_t ID, uint32_t adcVal) {
	int temp;
	float r = ( int) calcNTC(adcVal);  // adcVal;

	if (r > 100)
	{
		temp = (int) 100 * resToTemp(r);  // fill buffer with temperatures

		if (NTCaveragingBuffer[ID][0] == 0)
		{
			for (int n = 0; n < NTCAVERAGING; n++)  // fill buffer at startup
				NTCaveragingBuffer[ID][n] = temp;
		}
		else
			NTCaveragingBuffer[ID][tabPntr[ID]] = temp;

		tabPntr[ID]++;
		if (tabPntr[ID] == NTCAVERAGING)
		{
			tabPntr[ID] = 0;
		}
		temperature[ID] = calcTemp(ID);
	}
	else
		temperature[ID] = 9999;


	return temperature[ID];
}

//calculates temperatures from buffers

float calcTemp(uint32_t ID) {
	int32_t * pntr;
	int32_t sum = 0;
	uint8_t n;
	int32_t max = -0x7FFFF, min = 0x7FFFF;

	pntr = NTCaveragingBuffer[ID];

	if (NTCAVERAGING > 2)
	{
		for (n = 0; n < NTCAVERAGING; n++)
		{
			if (*pntr > max)
				max = *pntr;
			if (*pntr < min)
				min = *pntr;
			sum += *pntr++;
		}
		sum -= max;  // reject highest and lowest
		sum -= min;
		sum /= NTCAVERAGING - 2;
	}
	else
	{
		for (n = 0; n < NTCAVERAGING; n++)
		{
			sum += *pntr++;
		}
		sum /= NTCAVERAGING;
	}
	return ( sum/ 100.0);
}


