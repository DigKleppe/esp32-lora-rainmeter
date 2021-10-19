/*
 * scales.h
 *
 *  Created on: Oct 22, 2019
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_SCALES_H_
#define MAIN_INCLUDE_SCALES_H_


typedef enum { SCALES_RDY, SCALES_ERROR } ScalesStatus_t;
typedef struct {
	int counts;
	int weight;
	int totalWeight;
	float vBat;
	float vSolar;
	float temperature;
	uint32_t crc;
}scalesData_t ;

void scalesInit(void);
void scalesInitNoRAM( void);
bool scalesTask (void);
extern bool zeroScales; // flag forces zero action


#endif /* MAIN_INCLUDE_SCALES_H_ */
