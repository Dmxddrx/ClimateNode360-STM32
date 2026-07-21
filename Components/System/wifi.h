#ifndef WIFI_H_
#define WIFI_H_

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>
#include "rtc.h"

extern UART_HandleTypeDef huart2;

// Function Prototypes
int8_t WIFI_Init(void);
int8_t WIFI_Connect(const char* ssid, const char* password);
void WIFI_Disconnect(void);
void WIFI_SendCommand(const char* command);
int8_t WIFI_WaitForResponse(const char* expected_response, uint32_t timeout);
int8_t WIFI_GetIP(char* ip_out);

int8_t WIFI_StartUDP(const char* target_ip, uint16_t port);
int8_t WIFI_SendUDPData(const char* data);
int8_t WIFI_IsConnected(void);

int8_t WIFI_GetNTPTime(RTC_TimeTypeDef *rtc_time);

#endif /* WIFI_H_ */
