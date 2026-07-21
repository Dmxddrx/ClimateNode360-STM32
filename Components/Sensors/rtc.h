#ifndef RTC_H
#define RTC_H

#include "stm32f1xx_hal.h"

// Time structure to hold the current clock data
typedef struct {
    uint8_t Seconds;
    uint8_t Minutes;
    uint8_t Hour;
    uint8_t DayOfWeek; // 1-7
    uint8_t Date;      // 1-31
    uint8_t Month;     // 1-12
    uint8_t Year;      // 0-99 (represents 2000-2099)
} RTC_TimeTypeDef;

// Initialize the RTC by passing your I2C handle (e.g., &hi2c1)
void RTC_Init(I2C_HandleTypeDef *hi2c);

// Read and write functions
uint8_t RTC_GetTime(RTC_TimeTypeDef *time);
uint8_t RTC_SetTime(RTC_TimeTypeDef *time);

#endif /* RTC_H */
