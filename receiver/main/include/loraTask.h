/*
 * loraTask.h
 *
 *  Created on: Sep 27, 2021
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_LORATASK_H_
#define MAIN_INCLUDE_LORATASK_H_


typedef struct {
	uint16_t mssgCounts;
	uint16_t CRCerrs;
	int packetRssi;
	int packetSnr;

}loraInfo_t;

extern loraInfo_t loraInfo;
extern scalesData_t scalesData;

int printScalesData (char * p , scalesData_t * d );

#define LOGINTERVAL			 	5  // every 5 minutes new lora message


#define MAXLOGVALUES			(7 * 24 *4  )// 7 days every 15 minutes


#endif /* MAIN_INCLUDE_LORATASK_H_ */
