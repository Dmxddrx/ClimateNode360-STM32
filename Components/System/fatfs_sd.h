#ifndef FATFS_SD_H
#define FATFS_SD_H

#include "stm32f1xx_hal.h"
#include "diskio.h"

// Hardware Mapping
extern SPI_HandleTypeDef hspi1;
#define HSPI_SDCARD &hspi1
#define SD_CS_PORT GPIOA
#define SD_CS_PIN GPIO_PIN_4

// FatFs Bridge Functions
DSTATUS FATFS_SD_Init(void);
DSTATUS FATFS_SD_Status(void);
DRESULT FATFS_SD_Read(BYTE *buff, DWORD sector, UINT count);
DRESULT FATFS_SD_Write(BYTE *buff, DWORD sector, UINT count);
DRESULT FATFS_SD_Ioctl(BYTE cmd, void *buff);

#endif
