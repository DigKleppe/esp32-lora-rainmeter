/*
 * buffer.h
 *
 *  Created on: Feb 8, 2018
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_BUFFER_H_
#define MAIN_INCLUDE_BUFFER_H_

#include <stdint.h>

typedef struct {
	int * pBuffer;
	uint32_t bufSize;
	uint32_t bufValues;
	uint32_t bufWriteIndex;
	int32_t averageAccu;
	uint32_t averageValues;
	int32_t highest;
	int32_t lowest;
}buffer_t;

void* initBuffer ( buffer_t *pBuf, uint32_t bufferSize);
int writeBuffer( buffer_t *pBuf, int value);
float averageBuffer( buffer_t *pBuf);
void clearBuffer(  buffer_t *pBuf);
void deleteBuffer ( buffer_t *pBuf);



#endif /* MAIN_INCLUDE_BUFFER_H_ */
