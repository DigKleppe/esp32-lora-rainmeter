/*
 * i2cTask.h
 *
 *  Created on: Aug 9, 2021
 *      Author: dig
 *
 *      handles display and TMP117 (reference sensor)
 */

#ifndef MAIN_INCLUDE_I2CTASK_H_
#define MAIN_INCLUDE_I2CTASK_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

void displayTask(void *arg);
void initOled();

typedef struct {
	char * line1;
	char * line2;
	char * line3;
	char * line4;
	int timeToDisplay; // in ms
}displayMssg_t;

extern  xQueueHandle displayMssgbox;



#endif /* MAIN_INCLUDE_I2CTASK_H_ */
