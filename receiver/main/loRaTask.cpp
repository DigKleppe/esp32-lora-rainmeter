/*
 * loRaTask.cpp
 *
 *  Created on: Sep 27, 2021
 *      Author: dig
 */

/*	idf.py menuconfig
 *  mkdir -p build
 cd build
 cmake .. -G Ninja   # or 'Unix Makefiles'
 ninja

 esp-idf/components/freertos/include
 esp-idf/components/esp32/include
 esp-idf/components/lwip/include/lwip
 esp-idf/components/lwip/include/lwip/port
 esp-idf/components/driver/include
 */



#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_crc.h"
#include "Wire.h"

#include "scales.h"
#include "LoRa.h"
#include "main.h"
#include "loraTask.h"

uint8_t err;
scalesData_t scalesData;
loraInfo_t loraInfo;
extern int scriptState;



#define LORATXINTERVAL	5 // minutes
//#define LOGINTERVAL 	15

//void testLog(void) {
//	int len;
//	char buf[50];
////	logTxIdx = 0;
//	for (int p = 0; p < 20; p++) {
//
//		tLog[logTxIdx].timeStamp = timeStamp++;
//		for (int n = 0; n < NR_NTCS; n++) {
//
//			tLog[logTxIdx].temperature[n] = p + n;
//		}
//		tLog[logTxIdx].refTemperature = tmpTemperature; // from I2C TMP117
//		logTxIdx++;
//		if (logTxIdx >= MAXLOGVALUES)
//			logTxIdx = 0;
//	}
//
//	scriptState = 0;
//	do {
//		len = getLogScript(buf, 50);
//		buf[len] = 0;
//		printf("%s\r", buf);
//	} while (len);
//
//	for (int p = 0; p < 5; p++) {
//
//		tLog[logTxIdx].timeStamp = timeStamp++;
//		for (int n = 0; n < NR_NTCS; n++) {
//
//			tLog[logTxIdx].temperature[n] = p + n;
//		}
//		tLog[logTxIdx].refTemperature = tmpTemperature; // from I2C TMP117
//		logTxIdx++;
//		if (logTxIdx >= MAXLOGVALUES)
//			logTxIdx = 0;
//	}
//	do {
//		len = getNewMeasValuesScript(buf, 50);
//		buf[len] = 0;
//		printf("%s\r", buf);
//	} while (len);
//
//	printf("\r\n *************\r\n");
//
//}

void uartPrintScalesData(scalesData_t *scalesData) {
	printf("\n C:%d ", scalesData->mssgCounts);
	printf("w:%3.1f ", (float) scalesData->weight / 1000.0);
	printf("tw:%3.1f\t", (float) scalesData->totalWeight / 1000.0);
	printf("t:%2.2f ", scalesData->temperature);
	printf("vBat:%2.2f ", scalesData->vBat);
	printf("vSol:%2.2f ", scalesData->vSolar);
}

bool checkCRCscalesData(scalesData_t *scalesData) {
	uint32_t crc = esp_rom_crc32_le(0, (uint8_t const*) scalesData, sizeof(scalesData_t) - sizeof(uint32_t));
	return (crc == scalesData->crc);

}

void loRaTask(void *pvParameters) {
	long fErr;

	String info;
	scalesData_t tempScalesData;

	SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);

	LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);
	if (!LoRa.begin(BAND)) {
		Serial.println("Starting LoRa failed!");

	} else {
		LoRa.setSpreadingFactor(12);
		LoRa.setCodingRate4(8);
		LoRa.setSignalBandwidth(250000);
		LoRa.setPreambleLength(PREAMBLES);
		LoRa.receive(0);
	}



	while (1) {

		if (LoRa.parsePacket()) {
			uint8_t *recv = (uint8_t*) &tempScalesData;
			memset(&tempScalesData, 0, sizeof(tempScalesData));
			int rxCnt = 0;
			while (LoRa.available() && rxCnt++ < sizeof(tempScalesData)) {
				*recv++ = (char) LoRa.read();

			}
			uartPrintScalesData(&tempScalesData);
			if (!checkCRCscalesData(&tempScalesData)) {
				printf(" CRC error\n");
				loraInfo.CRCerrs++;
			} else {
				scalesData = tempScalesData;
//				tLog[logTxIdx].scalesData = scalesData;
//				logTxIdx++;
//				if (logTxIdx >= MAXLOGVALUES)
//					logTxIdx = 0;
			}

			fErr = LoRa.packetFrequencyError();
			printf(" frErr:%ld ", fErr);
			LoRa.setFrequency(LoRa.getFrequency() - fErr);
			double errPPM = (fErr * 1E6L) / LoRa.getFrequency();
			uint8_t offset = (uint8_t)(0.95 * errPPM);
			LoRa.writeRegister(0x27, offset);
			loraInfo.packetRssi = LoRa.packetRssi();
			loraInfo.packetSnr = LoRa.packetSnr();
			loraInfo.mssgCounts = (uint16_t) scalesData.mssgCounts;
			printf(" RSSI: %d", LoRa.packetRssi());
			printf(" SNR: %1.2f\n", LoRa.packetSnr());
			LoRa.receive(0);
		}
		vTaskDelay(100 / portTICK_RATE_MS);
	}

}

int printScalesData (char * p , scalesData_t * d ) {
	int len = sprintf ( p ,"%d,",d->mssgCounts);
	len += sprintf (p + len , "%3.1f,",(float)d->weight/1000.0);
	len += sprintf ( p + len ,"%3.1f,",(float)d->totalWeight/1000.0);
	len += sprintf ( p + len ,"%2.2f,",d->vBat);
	len += sprintf ( p + len ,"%2.2f,",d->vSolar);
	len += sprintf ( p + len ,"%2.2f,\n",d->temperature);
	return len;
}

// called from CGI

int getRTMeasValuesScript(char *pBuffer, int count) {
	int len = 0;

	switch (scriptState) {
		case 0:
		scriptState++;

		len = printScalesData ( pBuffer + len, &scalesData);

		return len;
		break;
		default:
		break;
	}
	return 0;
}


