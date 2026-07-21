#ifndef SD_H
#define SD_H

#include "fatfs.h"
#include "general.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Function Prototypes
uint8_t SD_Init(void);
uint8_t SD_LogData(float temperature, float humidity, int16_t dust, const char* status);
uint8_t SD_Format(void);

uint8_t SD_IsReady(void);
void SD_GetLogStats(uint32_t *rows, char *last_line);

#endif /* SD_H */
