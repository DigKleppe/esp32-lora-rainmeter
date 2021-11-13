/* Hello World Example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

/*

 cd ~/esp/esp-idf&&
 . ./export.sh&&
 cd /mnt/HDlinux/projecten/git/esp32-lora-rainmeter/transmitter/&&
 idf.py build


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

/*

 868.1 - SF7BW125 to SF12BW125
 868.3 - SF7BW125 to SF12BW125 and SF7BW250
 868.5 - SF7BW125 to SF12BW125
 867.1 - SF7BW125 to SF12BW125
 867.3 - SF7BW125 to SF12BW125
 867.5 - SF7BW125 to SF12BW125
 867.7 - SF7BW125 to SF12BW125
 867.9 - SF7BW125 to SF12BW125
 868.8 - FSK
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "Wire.h"

#include "driver/timer.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "soc/rtc.h"
#include "esp_pm.h"

#include "esp32/ulp.h"
#include "esp_rom_crc.h"

#include "scales.h"
#include "LoRa.h"
#include "main.h"

#include "ADS1X15.h"
#include "ntc.h"
#define PREAMBLES 				10  // nust be set at receiver side too

//define FAST					1
//

#define CYCLETIME_LOWBAT		300 // at low bat

#ifdef FAST
#define CYCLETIME				2 // every xxx seconds is measured
#define LORATXTIME				10 //sec lora messages
#else
#define CYCLETIME				60 // every xxx seconds is measured
#define LORATXTIME				300 // 5 minutes lora messages
#endif

#define VBATMIN					3.0 // below this value no activity

#define TIMER_DIVIDER       	16  //  Hardware timer clock divider 80Mhz
#define TIMER_SCALE           	(TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMERIDX			  	TIMER_0  // timer 0 group 0 used

//#define LED_BUILTIN			 	GPIO_NUM_16 	// 22 on TTGO 16	// 22 on TTGO

#define BAND 868E6				// 868Mhz

#define LORA_SENDER

#define NRAVGS 					16
#define I2C_MASTER_NUM          I2C_NUM_1

#define PRINT
const char FORMATSTR[] = "Lora rainsensor";
RTC_DATA_ATTR char formatStr[sizeof FORMATSTR];
RTC_DATA_ATTR int loraTxTmr;
RTC_DATA_ATTR int dayTimer;
RTC_DATA_ATTR int txCnts;

ADS1115 ADS(0x48);
//LoRaClass loRa();
uint8_t err;
#ifdef DMMBOARD
// Select the correct address setting
uint8_t ADDR_GND = 0x48;   // 1001000
uint8_t ADDR_VCC = 0x49;   // 1001001
uint8_t ADDR_SDA = 0x4A;   // 1001010
uint8_t ADDR_SCL = 0x4B;   // 1001011
uint8_t ADDR = ADDR_VCC;

//SSD1306Wire display(0x3C, OLED_SDA, OLED_SCL);
#endif

RTC_DATA_ATTR scalesData_t scalesData;
RTC_DATA_ATTR float vBat;

loraCommand_t command;
volatile bool commandSync;

//const int adcChannel[] = { 5,6,7 }; // ads115 mux  ANO is connected to 1/2VCC = Vref
#define REFCHANNEL 4  // = AN0 - gnd
#define NTCCHANNEL 5	// = AN1- gnd

void measureSystem(void) {
	float V;
	uint16_t read_raw;
	float vSolar;
	read_raw = ADS.readADC(0);   // VDD ADC = 3.7V , R Ref connected to 3,3V on AN0

	NTCreferenceValue = read_raw;

	read_raw = ADS.readADC(1);
	V = 4.096 * (float) read_raw / (float) 0x7FFF;

#ifdef PRINT
	//	printf(" %5.0f ", calcNTC(read_raw));
	//	printf(" %2.3f ", V);
	//	printf ( " %d %f",read_raw , calcNTC(read_raw) );
#endif
	if (V > 3.1) // open
		temperature[0] = 999;
	else {
		temperature[0] = updateNTC(0, read_raw);
	}
#ifdef DMMBOARD
//	tmpTemperature =  tmp.getTemperature();
//	err = Wire.lastError();
//	if (err) {
//		printf("\n%s ",Wire.getErrorText(err));
//		printScalesData();	}
//	sprintf ( str,"NTC: %2.1f", temperature[0]);
//
//	display.clear();
//	display.drawString(10, 0, str);
#else
	//	sprintf(str, "NTC: %2.2f ", temperature[0]);
#endif
	//printf("\n%s", str);

	read_raw = ADS.readADC(2); // Vbat
	vBat = 2 * 4.096 * (float) read_raw / (float) 0x7FFF;

#ifdef DMMBOARD
//	sprintf(str, "vBat: %2.2f ", vBat);
//	display.drawString(10, 15, str);
#endif
	//	printf("\%s", str);
	read_raw = ADS.readADC(3); // Solarpanel
	vSolar = 2 * 4.096 * (float) read_raw / (float) 0x7FFF;

#ifdef DMMBOARD
//	sprintf(str, "vSolar: %2.2f ", vSolar);
//	display.drawString(10, 30, str);
//	display.display();
#endif
	//printf("%s", str);

	scalesData.temperature = temperature[0];
	scalesData.vBat = vBat;
	scalesData.vSolar = vSolar;
}

void printScalesData(void) {
	printf("\n Tx:%d ", scalesData.counts);
	printf("\n C:%d ", scalesData.cycles);
	printf("w:%3.1f ", (float) scalesData.weight / 1000.0);
	printf("tw:%3.1f\t", (float) scalesData.totalWeight / 1000.0);
	printf("rw:%d\t", scalesData.rawWeight);
	printf("t:%2.2f ", scalesData.temperature);
	printf("vBat:%2.2f ", scalesData.vBat);
	printf("vSol:%2.2f ", scalesData.vSolar);
//	printf ( "crc:%d ",scalesData.crc);
}
void calcCRCscalesData(void) {
	scalesData.crc = esp_rom_crc32_le(0, (uint8_t const*) &scalesData, sizeof(scalesData) - sizeof(uint32_t));

}
void checkCRCscalesData(void) {
	uint32_t crc = esp_rom_crc32_le(0, (uint8_t const*) &scalesData, sizeof(scalesData) - sizeof(uint32_t));
	if (crc == scalesData.crc)
		printf("crc ok\n");
	else
		printf("crc error\n");
//	if ( esp_rom_crc32_le(0, (uint8_t const *)&scalesData, sizeof (  scalesData)) == 0)
//		printf("crc ok\n");
//	else {
//		printf("crc error\n");
}
int tstcntr;
extern "C" void app_main(void) {

	printf("loraTxRainSensor\n");

	Wire.begin(OLED_SDA, OLED_SCL, 400000); // also for ADC
	SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
	LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);

	scalesInit();
	if (strcmp(formatStr, FORMATSTR) != 0) {
		strcpy(formatStr, FORMATSTR); // powerup
		scalesInitNoRAM();
		loraTxTmr = 0;
		txCnts = 0;
		dayTimer = 0;
		Serial.println("NOVRAM init");
	}
//	loraTxTmr =  LORATXTIME/CYCLETIME;
#ifndef DMMBOARD
	ADS.begin();
	ADS.setGain(1);      // 1  =  �4.096V
	ADS.setDataRate(0);  // slow
#endif

	dayTimer++;
#ifdef FAST
	if (dayTimer >= 5) {
#else
	if (dayTimer >= (24 * 60 *60) /CYCLETIME)  {
#endif
		dayTimer = 0;
		zeroScales = true; // force open valve and zero
	}

	while (1) {
#ifndef DMMBOARD
		if (vBat < VBATMIN) {
			measureSystem();
			if (vBat < VBATMIN) {
				esp_sleep_enable_timer_wakeup(CYCLETIME_LOWBAT * 1000000);
				esp_deep_sleep_start();
			}
		} else {
			if (!scalesTask()) {
				measureSystem();
#else
		scalesData.vBat++;
		scalesData.cycles++;
		scalesData.temperature = rand();

		if (!scalesTask()) {
#endif
				loraTxTmr++;
				if (loraTxTmr >= ( LORATXTIME / CYCLETIME)) {
					loraTxTmr = 0;

					scalesData.counts++;
					printf("TxCnts: %d\n", scalesData.counts);
					calcCRCscalesData();

					if (!LoRa.begin(BAND)) {
						Serial.println("Starting LoRa failed!");
						vTaskDelay(5000 / portTICK_RATE_MS);

					}
					LoRa.setSpreadingFactor(12);
					LoRa.setCodingRate4(8);
					LoRa.setSignalBandwidth(250000);
					LoRa.setPreambleLength(PREAMBLES);
					LoRa.beginPacket();
					LoRa.write((uint8_t*) &scalesData, sizeof(scalesData));
					LoRa.endPacket();

				}
				esp_sleep_enable_timer_wakeup(CYCLETIME * 1000000);
				esp_deep_sleep_start();

			}
		}
	} //	vTaskDelay(1);// / portTICK_RATE_MS);
}

