/*
 * main.h
 *
 *  Created on: Sep 9, 2019
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_MAIN_H_
#define MAIN_INCLUDE_MAIN_H_
#define CHECK1 4462
#define CHECK2 19061956

typedef struct {
	uint32_t check1;
	uint32_t command;
	uint32_t check2;
} loraCommand_t;

#define COMMAND_SYNC 1
extern volatile  bool commandSync;
#define BAND 868E6				// 868Mhz


//#define DMMBOARD				1


#ifdef DMMBOARD

#define CONFIG_CLK				18
#define CONFIG_MISO				19
#define CONFIG_MOSI				23
#define CONFIG_NSS				GPIO_NUM_32  // relay
#define CONFIG_RST				16 //tp17
#define CONFIG_DIO0				GPIO_NUM_4 // tp18
// io 25 and 26 used for HX711 scale, connected to pin 7 / 8 J8 extension connector


#define	OLED_SDA				21
#define OLED_SCL				22

const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;

#else

#define OLED_SDA    4
#define OLED_SCL    15
#define OLED_RST    GPIO_NUM_16
#define CONFIG_MOSI 27
#define CONFIG_MISO 19
#define CONFIG_CLK  5
#define CONFIG_NSS  GPIO_NUM_18
#define CONFIG_RST  14
#define CONFIG_DIO0 26

const int LOADCELL_DOUT_PIN = 21;
const int LOADCELL_SCK_PIN = 13;

#define A1_INPIN			GPIO_NUM_22  // bridge
#define A2_INPIN			GPIO_NUM_25  // bridge
#define B1_INPIN			GPIO_NUM_17  // temporaray shorts endSWitches
#define B2_INPIN			GPIO_NUM_16  // temporaray shorts endSWitches

#endif

#define I2C_MASTER_SCL_IO          OLED_SCL               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO          OLED_SDA              /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM             I2C_NUM_1        /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ         400000           /*!< I2C master clock frequency */



#endif /* MAIN_INCLUDE_MAIN_H_ */
