/*
 * scales.cpp
 *
 *  Created on: Oct 22, 2019
 *      Author: dig
 */

#include "HX711.h"
#include "buffer.h"
#include "scales.h"
#include "driver/gpio.h"
#include "main.h"
#include "LoRa.h"
#ifdef DMMBOARD
#include <Wire.h>
#include <MCP23017.h>

#endif

//#define PRINT

#define SENSORDIA		12// cm
#define gPermm			(1000.0 * SENSORDIA * SENSORDIA * 3.1416 / (4*100*100))  // 1mm is 1000g per sq m

#define WEIGHTBUFFERSIZE 	32
//#define WEIGHTBUFFERSIZE 	4
#define CALFACTOR	   (100.0/49.4)   // to get reading in mg

#define MOTORONTIME			1
#define DRAINOPENTIME		5	// seconds
#define CLOSETIME			3	// maximum time to close
//#define FULLTIME			10  // seconds above FULL_LEVEL to open the drain;
#define FULL_LEVEL			130 // grams
// drain ball valve driven by a H-bridge
// has 2 endcontacts in series
// temporary bypassed by a second half-H bridge

#define BYPASSTIME 			2
// driver DRV8833 pins
#define PRINT	1

#ifdef DMMBOARD
#define MCP23017_ADDR 0x20
MCP23017 mcp = MCP23017(MCP23017_ADDR);
#endif

typedef enum {
	MC_OFF, MC_OPENC, MC_OPEN, MC_CLOSEC, MC_CLOSE
} motorCntr_t;
//LoRaClass loRa();

HX711 scale;
buffer_t weightBuffer;
scalesData_t *pScalesData; // = (scalesData_t*) pvParameters;
bool zeroScales; // flag forces zero action

extern scalesData_t scalesData;

static RTC_DATA_ATTR int state;
static RTC_DATA_ATTR int avgCntr;
static RTC_DATA_ATTR int zero;
static RTC_DATA_ATTR int stepTimer;

void motorControl(motorCntr_t motorCntr) {
	bool Ain1 = false, Ain2 = false, Bin1 = false, Bin2 = false;

	switch (motorCntr) {
	case MC_OFF:
		Ain1 = false;
		Ain2 = false;
		Bin1 = false;
		Bin2 = false;
		//	printf( "\nMC_OFF");
		break;

	case MC_CLOSEC:
		Ain1 = true;  // both bridges forward
		Ain2 = false;
		Bin1 = true;
		Bin2 = false;
//		printf( "\nMC_CLOSEC");
		break;

	case MC_CLOSE:
		Ain1 = true;   // 1 bridge forward
		Ain2 = false;
		Bin1 = false; // contact bypass off
		Bin2 = false;
		//	printf( "\nMC_CLOSE");
		break;

	case MC_OPENC:
		Ain1 = false;  //  both bridges reverse
		Ain2 = true;
		Bin1 = false;
		Bin2 = true;
//		printf( "\nMC_OPENC");
		break;

	case MC_OPEN:
		Ain1 = false;   // 1 bridge reverse
		Ain2 = true;
		Bin1 = false;  // contact bypass reverse off
		Bin2 = false;
//		printf( "\nMC_OPEN");
		break;
	}

#ifdef DMMBOARD
	uint8_t a = Ain1 + (Ain2<<1) + (Bin1 << 2) +(Bin2<<3);
	mcp.writePort(MCP23017Port::A, a);
#else
	gpio_set_level(A1_INPIN, Ain1);
	gpio_set_level(A2_INPIN, Ain2);
	gpio_set_level(B1_INPIN, Bin1);
	gpio_set_level(B2_INPIN, Bin2);
#endif

}

void scalesInit(void) {
	pScalesData = &scalesData;

	//initBuffer(&weightBuffer, WEIGHTBUFFERSIZE);
#ifdef DMMBOARD
		uint8_t err;
	    mcp.init();
	    mcp.portMode(MCP23017Port::A, 0);          //Port A as output
	    err = Wire.lastError();
	    if ( err)
	    	printf ( "I2C err: %s",Wire.getErrorText( err));

	#else
	gpio_pad_select_gpio (A1_INPIN);
	gpio_set_direction(A1_INPIN, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio (A2_INPIN);
	gpio_set_direction(A2_INPIN, GPIO_MODE_OUTPUT);

	gpio_pad_select_gpio (B1_INPIN);
	gpio_set_direction(B1_INPIN, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio (B2_INPIN);
	gpio_set_direction(B2_INPIN, GPIO_MODE_OUTPUT);
#endif

	scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

}

void scalesInitNoRAM(void) {
	state = 0;
	stepTimer = 0;
	zero = 0;
}

bool scalesTask() {

	static uint32_t reading = 0;
	//static int fullTimer = FULLTIME;
	static int weigth = 0;
	static RTC_DATA_ATTR int stepTimer = 0;

	if (commandSync) {  // received from base station to open the drain
		state = 0;
		pScalesData->totalWeight = 0;
		commandSync = false;
	}
	printf("state: %d \n", state);
	if (stepTimer)
		stepTimer--;
	else {
		switch (state) {
		case 0:
			motorControl(MC_OPENC); // drain valve open
			stepTimer = BYPASSTIME * 10;
			state++;
			break;
		case 1:
			motorControl(MC_OPEN);  // bypass endswitches off
			stepTimer = MOTORONTIME * 10;
			state++;
			break;

		case 2:  // drain is open
			motorControl(MC_OFF);
			stepTimer = DRAINOPENTIME * 10;
			state++;
			break;

		case 3:
			motorControl(MC_CLOSEC); // drain valve close after draining
			stepTimer = BYPASSTIME * 10;
			state++;
			break;
		case 4:
			motorControl(MC_CLOSE); // drain valve close
			stepTimer = CLOSETIME * 10;
			state++;
			break;

		case 5:
			motorControl(MC_OFF);\
			clearBuffer(&weightBuffer);
			state = 10;
			stepTimer = 1 * 10;
			avgCntr = 0;
			initBuffer(&weightBuffer, WEIGHTBUFFERSIZE);
			break;

		case 10:			 //zero reading after valve is closed
			if (scale.is_ready()) {
				reading = scale.read();
				writeBuffer(&weightBuffer, reading);
				avgCntr++;
				if (avgCntr >= WEIGHTBUFFERSIZE) {  // full buffer read
					zero = (int) averageBuffer(&weightBuffer);
					deleteBuffer(&weightBuffer);

					printf("zero: %d \n", zero);

					state++;
					avgCntr = 0;
					//	fullTimer = FULLTIME * 10;  // sec
				}
			}
			break;

		case 11:  // normal state entered after sleep
		{
			avgCntr = 0;
			initBuffer(&weightBuffer, WEIGHTBUFFERSIZE);
			do {
				if (scale.is_ready()) {
					reading = scale.read();
					//2	printf("\n reading: %d %d\n", reading, avgCntr);
					writeBuffer(&weightBuffer, reading);
					avgCntr++;
				}
			} while (avgCntr < WEIGHTBUFFERSIZE);

			weigth = (int) (averageBuffer(&weightBuffer) - zero);    // mg
			deleteBuffer(&weightBuffer);
#ifdef PRINT
			printf("raw: %d \t", weigth);
#endif
			weigth *= CALFACTOR;
			pScalesData->weight = weigth;

#ifdef PRINT
			float f = (float) weigth / 1000;
			float mm = f / gPermm;
			printf("%3.3f \t mm: %f\n", f, mm);
#endif
			if (weigth > (FULL_LEVEL * 1000) || zeroScales) {
				pScalesData->totalWeight += weigth;
				pScalesData->weight = 0;
				state = 0; // open the gate!
				zeroScales = false;
			}
			if (weigth < -2 * 1000) {  // negative weigth, miracle has happend
				state = 10; // zero
				//state = 0; // open the gate!				}
			}
		}
			break;

		default:
			state++;
			break;

		}
	}
	if (state == 11) //
		return (0); // normal nothing to do , can do sleep
	else {
		vTaskDelay(100 / portTICK_RATE_MS);

		return (1); // busy, no sleep

	}
}

//void scalesTask (void *pvParameters) {
//
//
//	int state = 0;
//	int avgCntr = 0;
//	int zero = 0;
//	uint32_t reading =0;
//	int fullTimer = (FULLTIME * 10);
//	int weigth= 0;
//	int stepTimer=0;
//
//	HX711 scale;
//	buffer_t weightBuffer;
//	scalesData_t* pScalesData = (scalesData_t*) pvParameters;
//
//	initBuffer( &weightBuffer, WEIGHTBUFFERSIZE );
//#ifdef DMMBOARD
//	uint8_t err;
//    mcp.init();
//    mcp.portMode(MCP23017Port::A, 0);          //Port A as output
//    err = Wire.lastError();
//    if ( err)
//    	printf ( "I2C err: %s",Wire.getErrorText( err));
//
//#else
//	gpio_pad_select_gpio(A1_INPIN);
//	gpio_set_direction(A1_INPIN, GPIO_MODE_OUTPUT);
//	gpio_pad_select_gpio(A2_INPIN);
//	gpio_set_direction(A2_INPIN, GPIO_MODE_OUTPUT);
//
//	gpio_pad_select_gpio(B1_INPIN);
//	gpio_set_direction(B1_INPIN, GPIO_MODE_OUTPUT);
//	gpio_pad_select_gpio(B2_INPIN);
//	gpio_set_direction(B2_INPIN, GPIO_MODE_OUTPUT);
//#endif
//
//    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
//
//	while (1)
//
//	{
//		vTaskDelay(100 / portTICK_RATE_MS);
//		if(  commandSync){  // received from base station to open the drain
//			state = 0;
//			pScalesData->totalWeight = 0;
//			commandSync = false;
//		}
//
//		if (stepTimer )
//			stepTimer--;
//		else {
//			switch ( state) {
//			case 0:
//				motorControl(MC_OPENC); // drain valve open
//
//				stepTimer = BYPASSTIME*10;
//				state++;
//				break;
//			case 1:
//				motorControl(MC_OPEN);  // bypass endswitches off
//				stepTimer = MOTORONTIME*10;
//				state++;
//				break;
//
//			case 2:  // drain is open
//				motorControl(MC_OFF);
//				stepTimer = DRAINOPENTIME*10;
//				state++;
//				break;
//
//			case 3:
//				motorControl(MC_CLOSEC); // drain valve close after draining
//				stepTimer = BYPASSTIME*10;
//				state++;
//				break;
//			case 4:
//				motorControl(MC_CLOSE); // drain valve close
//				stepTimer = CLOSETIME*10;
//				state++;
//				break;
//
//			case 5:
//				motorControl(MC_OFF);
//				clearBuffer(&weightBuffer);
//				state = 10;
//				stepTimer = 1*10;
//				break;
//
//			case 10:			 //zero reading after valve is closed
//				if (scale.is_ready()) {
//					reading = scale.read();
//					writeBuffer(&weightBuffer, reading );
//					avgCntr++;
//					if (avgCntr == WEIGHTBUFFERSIZE) {  // full buffer read
//						zero = (int) averageBuffer(&weightBuffer);
//
//						printf("zero: %d \n", zero);
//
//						state++;
//						avgCntr =0;
//						fullTimer = FULLTIME *10;  // sec
//					}
//				}
//				break;
//
//			case 11:
//				if (scale.is_ready()) {
//					reading = scale.read();
//			//		printf("\n reading: %d \n",reading);
//					writeBuffer(&weightBuffer, reading );
//					if (++avgCntr == WEIGHTBUFFERSIZE) {
//						avgCntr = 0;
//						weigth =(int) (averageBuffer(&weightBuffer)- zero);    // mg
//#ifdef PRINT
//						printf("raw: %d \t",weigth);
//#endif
//						weigth *= CALFACTOR;
//						pScalesData->weight = weigth;
//
//#ifdef PRINT
//						float f = (float) weigth/1000;
//						float mm  = f / gPermm;
//						printf("%3.3f \t mm: %f\n",f,mm);
//#endif
//						pScalesData->counts++;
//					}
//				}
//				if (weigth > (FULL_LEVEL * 1000)) {
//					fullTimer--;
//					if ( fullTimer == 0) {
//						pScalesData->totalWeight += weigth;
//						pScalesData->weight = 0;
//						pScalesData->cycles++;
//						state = 0; // open the gate!
//					}
//				}
//				else
//					fullTimer = FULLTIME*10;
//				break;
//
//			default:
//				state++;
//				break;
//
//			}
//		}
//	}
//}
