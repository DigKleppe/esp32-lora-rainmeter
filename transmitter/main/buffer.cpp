/*
 * buffer.c
 *
 *  Created on: Feb 8, 2018
 *      Author: dig
 */


#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

void* initBuffer ( buffer_t *pBuf, uint32_t bufferSize){
	pBuf->averageAccu = 0;
	pBuf->lowest = INT_MAX;
	pBuf->highest = INT_MIN;
	pBuf->averageValues = 0;
	pBuf->bufWriteIndex = 0;
	pBuf->bufSize = bufferSize;
	pBuf->pBuffer = (int *) malloc ( bufferSize * sizeof (int));
	return pBuf->pBuffer;
}

void deleteBuffer ( buffer_t *pBuf) {
	free (pBuf->pBuffer );
}
// write cyclic buffer#include <limits.h>
int writeBuffer( buffer_t *pBuf, int value){
	if ( pBuf -> pBuffer == NULL){
		return -1;
	}
	else {
		pBuf->pBuffer[pBuf->bufWriteIndex] = value;
		if ( pBuf->bufValues < pBuf->bufSize)
			pBuf->bufValues++;

		pBuf->bufWriteIndex++;
		if( pBuf->bufWriteIndex == pBuf->bufSize)
			pBuf->bufWriteIndex = 0;

		pBuf->averageAccu += value;
		pBuf->averageValues++;

		if (value > pBuf->highest)
			pBuf->highest = value;
		if (value < pBuf->lowest)
			pBuf->lowest = value;
	}
	return 0;
}


float averageBuffer( buffer_t *pBuf){
	float result = 0;
	if (pBuf->averageValues > 0 ){
		if ( pBuf->averageValues > 2 ) {
			pBuf->averageAccu -= pBuf->lowest;
			pBuf->averageAccu -= pBuf->highest;
			result = (float) pBuf->averageAccu/(pBuf->averageValues-2);
		}
		else
			result = (float) pBuf->averageAccu/pBuf->averageValues;

		pBuf->lowest = INT_MAX;
		pBuf->highest = INT_MIN;
		pBuf->averageAccu = 0;
		pBuf->averageValues = 0;
	}
	return result;
}

void clearBuffer(  buffer_t *pBuf) {
	pBuf->lowest = INT_MAX;
	pBuf->highest = INT_MIN;
	pBuf->averageAccu = 0;
	pBuf->averageValues = 0;

}
