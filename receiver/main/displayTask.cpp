#include "esp_system.h"

#include <cstring>
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "main.h"
#include "SSD1306Wire.h"
#include "displayTask.h"

//SSD1306Wire display(0x3C, OLED_SDA, OLED_SCL,GEOMETRY_128_64, I2C_ONE, 100000);

SSD1306Wire display(0x3C, OLED_SDA, OLED_SCL);
xQueueHandle displayMssgbox;

void initOled() {

	display.init();
	display.flipScreenVertically();
	display.clear();
	display.setFont(ArialMT_Plain_16);
	display.setTextAlignment(TEXT_ALIGN_LEFT);
}

void displayTask(void *arg) {

	displayMssg_t displayMssg;
	displayMssgbox = xQueueCreate(5, sizeof(displayMssg));
	String str;
	int cntr = 0;

//	display.init();
//	display.flipScreenVertically();
//	display.clear();
//	display.setFont(ArialMT_Plain_16);
//	display.setTextAlignment(TEXT_ALIGN_LEFT);
	//Wire.begin(OLED_SDA,OLED_SCL,100000);

	vTaskDelay(10 / portTICK_PERIOD_MS);
	while (1) {
		if (xQueueReceive(displayMssgbox, &displayMssg, 0)) { // anything else to display?
			display.clear();
			str = cntr++;
	//		display.drawString(1, 1, str);
//			display.drawString(0, 14,"DDDD" );
//			display.drawString(0, 28,"  line 3  " );
//			display.drawString(0, 42, " **LINE 4**");

//			if (strlen(displayMssg.line1) > 0) {
//				display.drawString(0, 0, displayMssg.line1);
//			}
//			if (strlen(displayMssg.line2) > 0) {
//				display.drawString(0, 20, displayMssg.line2);
//			}
//			if (strlen(displayMssg.line3) > 0) {
//				display.drawString(0, 40, displayMssg.line3);
//			}
//			if (strlen(displayMssg.line4) > 0) {
//				display.drawString(0, 42, displayMssg.line4);
//			}
		//	vTaskDelay(10 / portTICK_PERIOD_MS);
			display.display();
			if (displayMssg.timeToDisplay > 0) {
				vTaskDelay(displayMssg.timeToDisplay / portTICK_PERIOD_MS);
				display.clear();
		//		vTaskDelay(10 / portTICK_PERIOD_MS);
				display.display();
			}
		}
	}
}

