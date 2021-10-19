/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


/* mkdir -p build
	cd build
	cmake .. -G Ninja   # or 'Unix Makefiles'
	ninja
*/

#include <stdio.h>

//#include "../components/httpk/include/httpserver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "test.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "/Fonts/FreeMono12pt7b.h"
#include "../components/Adafruit-GFX-Library-master/Fonts/FreeSans18pt7b_Figs.h"



#define TIMER_DIVIDER         16  //  Hardware timer clock divider 80Mhz
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMERIDX			  TIMER_0  // timer 0 group 0 used

#define LED_BUILTIN GPIO_NUM_16 	// 22 on TTGO 	// 22 on TTGO

Adafruit_SSD1306 display(0);





#include "LoRa.h"

int t;

extern "C" void app_main(void)
{

	LoRaClass LoRa;
	printf("Hello world!\n");
	t = test(t);
	LoRa.begin(868);

	printf("Hello world! test: %d\n",t);

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
