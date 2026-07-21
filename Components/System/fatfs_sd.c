#include "fatfs_sd.h"
#include "general.h"

#define TRUE  1
#define FALSE 0
#define SD_DEFAULT_BLOCK_SIZE 512

/* SD Card Commands */
#define CMD0   (0)      /* GO_IDLE_STATE */
#define CMD8   (8)      /* SEND_IF_COND */
#define CMD12  (12)     /* STOP_TRANSMISSION */  // <--- ADD THIS
#define CMD16  (16)     /* SET_BLOCKLEN */
#define CMD17  (17)     /* READ_SINGLE_BLOCK */
#define CMD24  (24)     /* WRITE_BLOCK */
#define CMD55  (55)     /* APP_CMD */
#define ACMD41 (41)     /* SD_SEND_OP_COND */
#define CMD58  (58)     /* READ_OCR */

static volatile DSTATUS Stat = STA_NOINIT;
static uint8_t CardType;

/* SPI Wrapper Functions */
static void SPI_TxByte(uint8_t data) {
    HAL_SPI_Transmit(HSPI_SDCARD, &data, 1, 100);
}

static uint8_t SPI_RxByte(void) {
    uint8_t dummy = 0xFF, data;
    HAL_SPI_TransmitReceive(HSPI_SDCARD, &dummy, &data, 1, 100);
    return data;
}

static void SD_Select(void) {
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
}

static void SD_Deselect(void) {
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
    SPI_RxByte(); // Dummy clock to force DO release
}

static uint8_t SD_WaitReady(void) {
    uint8_t res;
    uint32_t timeout = HAL_GetTick() + 500;
    do {
        res = SPI_RxByte();
    } while (res != 0xFF && HAL_GetTick() < timeout);
    return res;
}

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg) {
    uint8_t n, res;
    if (SD_WaitReady() != 0xFF) return 0xFF;

    SPI_TxByte(cmd | 0x40);
    SPI_TxByte((uint8_t)(arg >> 24));
    SPI_TxByte((uint8_t)(arg >> 16));
    SPI_TxByte((uint8_t)(arg >> 8));
    SPI_TxByte((uint8_t)arg);

    n = 0x01; // Dummy CRC
    if (cmd == CMD0) n = 0x95;
    if (cmd == CMD8) n = 0x87;
    SPI_TxByte(n);

    if (cmd == CMD12) SPI_RxByte();
    n = 10;
    do {
        res = SPI_RxByte();
    } while ((res & 0x80) && --n);
    return res;
}

DSTATUS FATFS_SD_Init(void) {
    uint8_t n, cmd, ty, ocr[4];
    uint32_t timeout;

    SD_Deselect();
	HAL_Delay(100); // Give the 5V regulator time to stabilize

	// Send 160 dummy clocks instead of 80 just to be safe
	for (n = 20; n; n--) SPI_RxByte();

    SD_Select();

    ty = 0;
    if (SD_SendCmd(CMD0, 0) == 1) { // Put card in SPI mode
        timeout = HAL_GetTick() + 1000;
        if (SD_SendCmd(CMD8, 0x1AA) == 1) { // SDv2
            for (n = 0; n < 4; n++) ocr[n] = SPI_RxByte();
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {

                // --- CRITICAL BUG FIX 1: Send CMD55 before ACMD41 ---
                while (HAL_GetTick() < timeout) {
                    SD_SendCmd(CMD55, 0);
                    if (SD_SendCmd(ACMD41, 1UL << 30) == 0) break;
                }
                // ----------------------------------------------------

                if (HAL_GetTick() < timeout && SD_SendCmd(CMD58, 0) == 0) {
                    for (n = 0; n < 4; n++) ocr[n] = SPI_RxByte();
                    ty = (ocr[0] & 0x40) ? 3 : 2;
                }
            }
        } else { // SDv1 or MMC
            ty = (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(ACMD41, 0) <= 1) ? 2 : 1;
            cmd = (ty == 2) ? ACMD41 : 1; // 1 = CMD1

            // --- CRITICAL BUG FIX 2: Send CMD55 before ACMD41 ---
            while (HAL_GetTick() < timeout) {
                if (cmd == ACMD41) SD_SendCmd(CMD55, 0);
                if (SD_SendCmd(cmd, 0) == 0) break;
            }
            // ----------------------------------------------------

            if (HAL_GetTick() >= timeout || SD_SendCmd(CMD16, SD_DEFAULT_BLOCK_SIZE) != 0) ty = 0;
        }
    }
    CardType = ty;
    SD_Deselect();
    if (ty) Stat &= ~STA_NOINIT; // Initialization successful
    else Stat |= STA_NOINIT;
    return Stat;
}

DSTATUS FATFS_SD_Status(void) {
    return Stat;
}

DRESULT FATFS_SD_Read(BYTE *buff, DWORD sector, UINT count) {
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    if (CardType != 3) sector *= SD_DEFAULT_BLOCK_SIZE; // Convert to byte address if not SDHC

    // --- NEW: Feed the watchdog during heavy SD reads! ---
	HAL_IWDG_Refresh(&hiwdg);

    SD_Select();
    if (SD_SendCmd(CMD17, sector) == 0) {
        uint32_t timeout = HAL_GetTick() + 100;
        while (SPI_RxByte() != 0xFE && HAL_GetTick() < timeout); // Wait for data token
        if (HAL_GetTick() < timeout) {
            for (uint16_t i = 0; i < SD_DEFAULT_BLOCK_SIZE; i++) *buff++ = SPI_RxByte();
            SPI_RxByte(); SPI_RxByte(); // Discard CRC
            SD_Deselect();
            return RES_OK;
        }
    }
    SD_Deselect();
    return RES_ERROR;
}

DRESULT FATFS_SD_Write(BYTE *buff, DWORD sector, UINT count) {
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    if (CardType != 3) sector *= SD_DEFAULT_BLOCK_SIZE;

    // --- NEW: Feed the watchdog during heavy SD writes! ---
	HAL_IWDG_Refresh(&hiwdg);

    SD_Select();
    if (SD_SendCmd(CMD24, sector) == 0) {
        SPI_TxByte(0xFE); // Data token
        for (uint16_t i = 0; i < SD_DEFAULT_BLOCK_SIZE; i++) SPI_TxByte(*buff++);
        SPI_RxByte(); SPI_RxByte(); // Dummy CRC
        if ((SPI_RxByte() & 0x1F) == 0x05) { // Check data accepted
            SD_WaitReady();
            SD_Deselect();
            return RES_OK;
        }
    }
    SD_Deselect();
    return RES_ERROR;
}

DRESULT FATFS_SD_Ioctl(BYTE cmd, void *buff) {
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    DRESULT res = RES_ERROR;

    SD_Select();
    switch (cmd) {
        case CTRL_SYNC:
            if (SD_WaitReady() == 0xFF) res = RES_OK;
            break;

        case GET_SECTOR_SIZE:
            *(WORD*)buff = SD_DEFAULT_BLOCK_SIZE; // Always 512 bytes
            res = RES_OK;
            break;

        case GET_SECTOR_COUNT: {
            uint8_t csd[16];

            // Send CMD9 (SEND_CSD) to read the Card-Specific Data register
            if (SD_SendCmd(9, 0) == 0) {
                uint32_t timeout = HAL_GetTick() + 100;

                // Wait for the 0xFE data token
                while (SPI_RxByte() != 0xFE && HAL_GetTick() < timeout);

                if (HAL_GetTick() < timeout) {
                    for (uint8_t i = 0; i < 16; i++) csd[i] = SPI_RxByte();
                    SPI_RxByte(); SPI_RxByte(); // Discard the CRC

                    // Calculate true capacity based on SD Card Version
                    if ((csd[0] >> 6) == 1) {
                        // SDHC / SDXC (Version 2.0 - High Capacity)
                        uint32_t c_size = csd[9] + ((uint32_t)csd[8] << 8) + ((uint32_t)(csd[7] & 63) << 16) + 1;
                        *(DWORD*)buff = c_size << 10;
                    } else {
                        // Standard SD (Version 1.0 - Low Capacity)
                        uint8_t n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                        uint32_t c_size = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
                        *(DWORD*)buff = c_size << (n - 9);
                    }
                    res = RES_OK;
                }
            }
            break;
        }

        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            res = RES_OK;
            break;
    }
    SD_Deselect();
    return res;
}
