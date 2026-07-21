#ifndef GENERAL_H
#define GENERAL_H

#include "main.h"
#include "oled.h"
#include "fatfs.h"
#include "oledGUI.h"
#include "wifi.h"
#include "sd.h"
#include "btns.h"
#include "sht30.h"
#include "dust.h"
#include "buzzer.h"
#include "led.h"
#include "rtc.h"

#include <stdio.h>
#include <string.h>


extern IWDG_HandleTypeDef hiwdg;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern SPI_HandleTypeDef hspi1;

// --- NETWORK CONFIGURATIONS ---
#define UDP_PORT    8080

#define WIFI_SSID_2 "ENTGRA 2.5G"
#define WIFI_PASS_2 "Entgra@110"
#define PC_IP_2     "192.168.8.198"

#define WIFI_SSID_1 "Dmx's Note20 Ultra"
#define WIFI_PASS_1 "11111129"
#define PC_IP_1     "10.184.253.199"

#define WIFI_SSID_3 "Dialog 4G 208"
#define WIFI_PASS_3 "Hasith2001"
#define PC_IP_3     "192.168.8.198"

#define WIFI_SSID_4 "ENTGRA 5G"
#define WIFI_PASS_4 "Entgra@110"
#define PC_IP_4     "192.168.8.198"

#define SD_FORMAT 1

// --- GLOBAL VARIABLES ---
extern uint8_t esp_is_ready;
extern uint8_t wifi_is_connected;
extern char current_ip[16];
extern char current_ssid[32];
extern char current_pc_ip[16];

// Function Prototypes
void General_Init(void);
void General_Run(void);
void Scan_I2C_Bus(I2C_HandleTypeDef *hi2c, char *buffer);

#endif /* GENERAL_H */
