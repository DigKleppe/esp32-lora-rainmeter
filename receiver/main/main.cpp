//  /home/dig/.espressif/tools/openocd-esp32/v0.10.0-esp32-20200709/openocd-esp32/bin/openocd -f interface/ftdi/esp32_devkitj_v1.cfg -f board/esp-wroom-32.cfg -c "program_esp /home/dig/projecten/esp32/thermometer/build/storage.bin 0x210000 verify"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include <string.h>
#include <time.h>
#include "WString.h"
#include <math.h>

#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "esp_sntp.h"

#include "driver/timer.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#include "main.h"

#include "scales.h"
#include "loraTask.h"
#include "displayTask.h"
#include "settings.h"

typedef struct {
//	time_t timeStamp;
	scalesData_t scalesData;
} log_t;

log_t tLog[MAXLOGVALUES];

int logTxIdx;
int logRxIdx;
extern int scriptState;

//#define SIMULATE 1

esp_err_t start_file_server(const char *base_path);
static const int CONNECTED_BIT = BIT0;
static const int WIFI_CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int WIFI_FAIL_BIT = BIT2;

void loRaTask(void *pvParameters);

gpio_num_t ALLGPIOS[] = {
		GPIO_NUM_25,
		GPIO_NUM_26,
		GPIO_NUM_27,
		GPIO_NUM_14,
		GPIO_NUM_12,
		GPIO_NUM_13,
		GPIO_NUM_23,
		GPIO_NUM_22,
		GPIO_NUM_21,
		GPIO_NUM_19,
		GPIO_NUM_18,
		GPIO_NUM_5,
		GPIO_NUM_17,
		GPIO_NUM_16,
		GPIO_NUM_4,
		GPIO_NUM_0,
		GPIO_NUM_2,
		GPIO_NUM_15 };

#define ESP_INTR_FLAG_DEFAULT 0

static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "smartconfig_example";

const char *wifi_network_ssid = "Klepnet";
const char *wifi_network_password = "xxxxxxxxxxx";

bool gotIp;
bool smartConfigEnabled = true; // after powerup and no wifi do smartconfig, else keep trying to make contact with existing values
extern uint16_t crcErrs;

char myIP[20];
int s_retry_num;
wifi_config_t wifi_config;

static void event_handler1(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < 2) {
			esp_wifi_connect();
			if (smartConfigEnabled)
				s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG, "connect to the AP fail");
		gotIp = false;
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		sprintf(myIP, IPSTR, IP2STR(&event->ip_info.ip));
		gotIp = true;
		smartConfigEnabled = false;
		s_retry_num = -10;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

static void smartconfig_example_task(void *parm) {
	EventBits_t uxBits;
	ESP_ERROR_CHECK (esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));smartconfig_start_config_t
	cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
	while (1) {
		uxBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
		if (uxBits & WIFI_CONNECTED_BIT) {
			ESP_LOGI(TAG, "WiFi Connected to ap");
			vTaskDelete (NULL);
		}
		if (uxBits & ESPTOUCH_DONE_BIT) {
			ESP_LOGI(TAG, "smartconfig over");
			esp_smartconfig_stop();

			vTaskDelete (NULL);
		}
	}
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		esp_wifi_connect();
		xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
		ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		sprintf(myIP, IPSTR, IP2STR(&event->ip_info.ip));
		gotIp = true;
	} else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
		ESP_LOGI(TAG, "Scan done");
	} else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
		ESP_LOGI(TAG, "Found channel");
	} else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
		ESP_LOGI(TAG, "Got SSID and password");

		smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t*) event_data;

		uint8_t ssid[33] = { 0 };
		uint8_t password[65] = { 0 };
		uint8_t rvd_data[33] = { 0 };

		bzero(&wifi_config, sizeof(wifi_config_t));
		memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
		memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
		wifi_config.sta.bssid_set = evt->bssid_set;
		if (wifi_config.sta.bssid_set == true) {
			memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
		}

		memcpy(ssid, evt->ssid, sizeof(evt->ssid));
		memcpy(password, evt->password, sizeof(evt->password));
		ESP_LOGI(TAG, "SSID:%s", ssid);
		ESP_LOGI(TAG, "PASSWORD:%s", password);
		if (evt->type == SC_TYPE_ESPTOUCH_V2) {
			ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
			ESP_LOGI(TAG, "RVD_DATA:");
			for (int i = 0; i < 33; i++) {
				printf("%02x ", rvd_data[i]);
			}
			printf("\n");
		}

		ESP_ERROR_CHECK (esp_wifi_disconnect());ESP_ERROR_CHECK
		(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
		esp_wifi_connect();
	} else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
		xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
	}
}

// for smartconfig
static void initialise_wifi(void) {

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

	ESP_ERROR_CHECK (esp_wifi_set_mode(WIFI_MODE_STA));ESP_ERROR_CHECK
	(esp_wifi_start());}

void wifi_init_sta(void) {

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler1, NULL, &instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler1, NULL, &instance_got_ip));

	memset(&wifi_config, 0, sizeof(wifi_config));
	strcpy((char*) &wifi_config.sta.ssid, wifi_network_ssid);
	strcpy((char*) &wifi_config.sta.password, wifi_network_password);
	if (strlen(userSettings.SSID) > 0)
		strcpy((char*) &wifi_config.sta.ssid, (const char*) userSettings.SSID);
	if (strlen(userSettings.pwd) > 0)
		strcpy((char*) &wifi_config.sta.password, (const char*) userSettings.pwd);

	wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	wifi_config.sta.pmf_cfg.capable = true;
	wifi_config.sta.pmf_cfg.required = false;

	ESP_ERROR_CHECK (esp_wifi_set_mode(WIFI_MODE_STA));ESP_ERROR_CHECK
	(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK (esp_wifi_start());

ESP_LOGI	(TAG, "wifi_init_sta finished.");

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
	pdFALSE,
	pdFALSE, portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_network_ssid, wifi_network_password);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_network_ssid, wifi_network_password);
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}

	//	/* The event will not be processed after unregister */
	//	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	//	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	//	vEventGroupDelete(s_wifi_event_group);
}

/* Function to initialize SPIFFS */
static esp_err_t init_spiffs(void) {
	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = { .base_path = "/spiffs", .partition_label = NULL, .max_files = 25,  // This decides the maximum number of files that can be created on the storage
			.format_if_mount_failed = true };

	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return ESP_FAIL;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	return ESP_OK;
}

void time_sync_notification_cb(struct timeval *tv) {
	ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void) {
	ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode (SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
//    sntp_setservername(0, "8.8.8.8");
	sntp_setservername(1, "1.1.1.1");
	sntp_setservername(2, "208.67.222.222");
	sntp_setservername(3, "95.85.36.28");

	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
	sntp_init();
}

// reads all avaiable data from log
// issued as first request.

int getLogScript(char *pBuffer, int count) {
	static int oldTimeStamp = 0;
	static int logsToSend = 0;
	int left, len = 0;
	int n;
	if (scriptState == 0) { // find oldest value in cyclic logbuffer
		logRxIdx = 0;
		oldTimeStamp = 0;
		if (tLog[logRxIdx].scalesData.mssgCounts == 0)
			return 0; // nothing present

		for (n = 0; n < MAXLOGVALUES; n++) {
			if (tLog[logRxIdx].scalesData.mssgCounts < oldTimeStamp)
				break;
			else {
				oldTimeStamp = tLog[logRxIdx++].scalesData.mssgCounts;
			}
		}
		if (tLog[logRxIdx].scalesData.mssgCounts == 0) { // then log not full
			// not written yet?
			logRxIdx = 0;
			logsToSend = n;
		} else
			logsToSend = MAXLOGVALUES;
		scriptState++;
	}
	if (scriptState == 1) { // send complete buffer
		if (logsToSend) {
			do {
				len += printScalesData(pBuffer + len, &tLog[logRxIdx].scalesData);
				logRxIdx++;
				if (logRxIdx >= MAXLOGVALUES)
					logRxIdx = 0;
				left = count - len;
				logsToSend--;

			} while (logsToSend && (left > 40));
		}
	}
	return len;
}

extern "C" void app_main(void) {

	int state = 0;
	String info;
	char strftime_buf[64];
	time_t now = 0;
	struct tm timeinfo = { 0 };
	int min = 0;

	setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
	tzset();

	int oldMessageCounts = -1;
	displayMssg_t displayMssg = { .line1 = (char*) "LoraRX", .line2 = (char*) "SW 0.0", .line3 = (char*) "***", .line4 = (char*) "***", .timeToDisplay = 2000 };

	init_spiffs();
	ESP_ERROR_CHECK (nvs_flash_init());
//	initOled();

xTaskCreate	(&loRaTask, "loRa", 35000, NULL, 2, NULL);
//	xTaskCreate(&displayTask, "display", 5000, NULL, 2, NULL);

	loadUserSettings();

	FILE *f = fopen("/spiffs/index.html", "r");
	if (f == NULL) {
		ESP_LOGE(TAG, "Failed to open index.html");
		displayMssg.line4 = (char*) "No webpage";

	} else {
		ESP_LOGE(TAG, "open index.html OK");
		displayMssg.line4 = (char*) "Wwbpage OK";
	}
	fclose(f);

	vTaskDelay(100);

//	xQueueSend(displayMssgbox, &displayMssg, 0);

#ifdef SIMULATE

	for (int n = 0; n < (2 * 24 * 4); n++) {

		scalesData.mssgCounts++;
		scalesData.temperature = 20 + 5 * sin(n / 10);
		scalesData.vBat = 5 + 1 * sin(n / 10);
		scalesData.vSolar = 6 + 1 * sin(n / 15);
		scalesData.totalWeight = 50000;
		if ((n & 16) > 12)
			scalesData.totalWeight += 20000;

		scalesData.weight = 10000;
		tLog[logTxIdx].scalesData = scalesData;
		logTxIdx++;
		if (logTxIdx >= MAXLOGVALUES)
			logTxIdx = 0;
	}

#endif

	while (1) {
		vTaskDelay(10);
		switch (state) {

		case 0:
			s_wifi_event_group = xEventGroupCreate();
			ESP_ERROR_CHECK (esp_netif_init());ESP_ERROR_CHECK(esp_event_loop_create_default());
			esp_netif_create_default_wifi_sta();

			wifi_init_sta(); // try to connect to previous AP
			state++;
			break;

			case 1:
			if (gotIp) {
				state = 2;

			} else {
				//			gpio_set_level(LED_PIN, 0);
				displayMssg.line1 = (char*) "Smartconfig";
				displayMssg.line2 = (char*) "Started";
				displayMssg.line3 = (char*) "";
				displayMssg.timeToDisplay = 5000;
				//	xQueueSend(displayMssgbox, &displayMssg, 0);
				esp_wifi_stop();
				initialise_wifi();
				state++;
			}
			break;

			case 2:
			//	gpio_set_level(LED_PIN, 1);
			if (gotIp) {

				ESP_LOGI(TAG, "WiFi Connected to ap");
				displayMssg.line1 = (char*) "Connected";
				displayMssg.line2 = (char*) wifi_config.sta.ssid;
				displayMssg.line3 = myIP;
				displayMssg.timeToDisplay = 2000;
				//		xQueueSend(displayMssgbox, &displayMssg, 0);
				start_file_server("/spiffs");

				initialize_sntp();

				bool changed = false;

				if (strcmp((const char*) userSettings.pwd, (const char*) wifi_config.sta.password) != 0) {
					strcpy(userSettings.pwd, (const char*) wifi_config.sta.password);
					changed = true;
				}
				if (strcmp((const char*) userSettings.SSID, (const char*) wifi_config.sta.ssid) != 0) {
					strcpy(userSettings.SSID, (const char*) wifi_config.sta.ssid);
					changed = true;
				}
				if (changed) {
					saveUserSettings();
				}
				state++;
			}

			break;

			case 3:
			case 5:
//			if (! gotIp ) {
//		//		gpio_set_level(LED_PIN, 0); // led flash if not connected anymore
//				vTaskDelay(10);
//		//		gpio_set_level(LED_PIN, 1);
//			}
//			else
//				gpio_set_level(LED_PIN, 1);

			time(&now);
			localtime_r(&now, &timeinfo);
			strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
			//	ESP_LOGI(TAG, "The current date/time: %s", strftime_buf);

			vTaskDelay(2000 / portTICK_PERIOD_MS);
//			if( scalesData.mssgCounts != oldMessageCounts) {
//				oldMessageCounts = scalesData.mssgCounts;
//				ESP_LOGI(TAG, "The current date/time: %s", strftime_buf);
//				//	tLog[logTxIdx].timeStamp = now;
//				tLog[logTxIdx].scalesData = scalesData;
//
//				logTxIdx++;
//				if (logTxIdx >= MAXLOGVALUES)
//				logTxIdx = 0;
//			}
			if ((timeinfo.tm_min % 15) == 0) { // log every 15 minutes
				if (min != timeinfo.tm_min) {
					min = timeinfo.tm_min;

					ESP_LOGI(TAG, "The current date/time: %s", strftime_buf);
					//	tLog[logTxIdx].timeStamp = now;
					tLog[logTxIdx].scalesData = scalesData;

					logTxIdx++;
					if (logTxIdx >= MAXLOGVALUES)
						logTxIdx = 0;
				}
			}
			break;
		}

		if (oldMessageCounts != scalesData.mssgCounts) {
			oldMessageCounts = scalesData.mssgCounts;

//
//			//   String info = "[" + String((received)) + "]" + "RSSI " + String(LoRa.packetRssi());
//			info =  "RSSI " + String(loraInfo.packetRssi) +" " +  String(loraInfo.packetSnr);
//			display.drawString(5,0, info);
//			info = "vB: " + String(scalesData.vBat) +" so:" +  String(scalesData.vSolar);
//			display.drawString(5,15, info);
//			float  weight = ( (float)scalesData.weight +  (float)scalesData.totalWeight)/1000.0;
//
//			info = String(weight ) + " " +  String(scalesData.temperature );
//			display.drawString(5,30, info);
//
//			info =   String(scalesData.mssgCounts) +  " E:" + String( crcErrs);
//			display.drawString(5,45, info);
//			display.display();
		}
	}
}

