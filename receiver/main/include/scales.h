/*
 * scales.h
 *
 *  Created on: Oct 22, 2019
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_SCALES_H_
#define MAIN_INCLUDE_SCALES_H_



typedef struct {
	int mssgCounts;
	int weight;
	int totalWeight;
	int rawWeight;
	float vBat;
	float vSolar;
	float temperature;
	int cycles;
	uint32_t crc;
}scalesData_t ;


void scalesTask (void *pvParameters);



#endif /* MAIN_INCLUDE_SCALES_H_ */
