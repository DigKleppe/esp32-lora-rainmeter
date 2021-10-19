/*
 * settings.c
 *
 *  Created on: Nov 30, 2017
 *      Author: dig
 */


#include "settings.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include <string.h>
#include <cerrno>

#define STORAGE_NAMESPACE "storage"

extern settingsDescr_t settingsDescr[];
//extern char * myIpAddr;
extern int myRssi;

userSettings_t userSettingsDefaults = {
		1,
		{"Klepnet"},
	//	{"xxxxxxxxxxx"},
		{"Yellowstone1999"},
		0,
		0,
		6000,
		{CHECKSTR}
};

userSettings_t userSettings;
calibrationSettings_t calibrationSettings;



settingsDescr_t settingsDescr[] = {
	//	{INT, 1, (void *)&userSettings.channel, 0 , 6},
	//	{STR, 1, (void *)&userSettings.SSID, 0,0},
	//	{STR, 1, (void *)&userSettings.myIpAddress, 0,0},
	//	{STR, 1, (void *)&userSettings.APSSID,0, 0},

		{INT ,1, (void *)&userSettings.averages, 1, 32},
		{INT ,1, (void *)&userSettings.conversionSpeed,1, 16},
		{INT ,1, (void *)&userSettings.udpPort,1, 65500},
		{STR, 0, (void *) 0, 0,0 } // last size 0
};

settingsDescr_t calibrationSettingsDesrc[] = {
//		{INT, NR_DMMCALS , (void *)calibrationSettings.DCoffset, 0,0 },
//		{INT, NR_DMMCALS , (void *)calibrationSettings.ACoffset ,0,0 },
//		{INT, NR_DMMCALS , (void *)calibrationSettings.ohmsOffset,0,0},
//		{FLT, NR_DMMCALS , (void *)calibrationSettings.calFactor,0,0},
		{STR, 0, (void *) 0,0,0} // last size 0
};

esp_err_t saveUserSettings(void)
{
	FILE *fd = fopen("/spiffs/settings", "wb");
	if (fd == NULL) {
		printf("  Error opening file (%d) %s\n", errno, strerror(errno));
		printf("\n");
		return-1;
	}
	int len = sizeof (userSettings_t);
	int res = fwrite( &userSettings, 1, len, fd);
	if (res != len) {
		printf("  Error writing to file(%d <> %d\n", res, len);
		res = fclose(fd);
		return -1;
	}
	res = fclose(fd);
	return 0;
}

esp_err_t loadUserSettings(){
	esp_err_t res = 0;
	FILE *fd = fopen("/spiffs/settings", "rb");
	if (fd == NULL) {
		printf("  Error opening settings file (%d) %s\n", errno, strerror(errno));

	}
	else {
		int len = sizeof (userSettings_t);
		res = fread( &userSettings, 1, len, fd);
		if (res <= 0) {
			printf("  Error reading from file\n");
		}
		res = fclose(fd);
	}
	if (strcmp(userSettings.checkstr, CHECKSTR) != 0 ){
		userSettings = userSettingsDefaults;
		printf( " ** defaults loaded");
		saveUserSettings();
	}
	return res;
}


esp_err_t saveCalibrationSettings(void)
{
	FILE *fd = fopen("/spiffs/calsettings", "wb");
	if (fd == NULL) {
		printf("  Error opening file (%d) %s\n", errno, strerror(errno));
		printf("\n");
		return-1;
	}
	strcpy (calibrationSettings.checkstr, CALCHECKSTR);

	int len = sizeof (calibrationSettings_t);
	int res = fwrite( &calibrationSettings, 1, len, fd);
	if (res != len) {
		printf("  Error writing to file(%d <> %d\n", res, len);
		res = fclose(fd);
		return -1;
	}
	res = fclose(fd);
	return 0;
}

esp_err_t loadCalibrationSettings(){
	esp_err_t res = 0;
	FILE *fd = fopen("/spiffs/calsettings", "rb");
	if (fd == NULL) {
		printf("  Error opening settings file (%d) %s\n", errno, strerror(errno));

	}
	else {
		int len = sizeof (calibrationSettings_t);
		res = fread( &calibrationSettings, 1, len, fd);
		if (res <= 0) {
			printf("  Error reading from file\n");
		}
		res = fclose(fd);
	}
//	if (strcmp(calibrationSettings.checkstr, CALCHECKSTR) != 0 ){
//		memset (&calibrationSettings , 0, sizeof ( calibrationSettings_t));
////		for (int n = 0; n < NR_RANGES; n++){
////			calibrationSettings.calFactor[n] = 10.0;
////		}
////		calibrationSettings.ACcalFactor = 1.0;
////		printf( " ** defaults loaded");
//		saveCalibrationSettings();
//	}
	return res;
}
