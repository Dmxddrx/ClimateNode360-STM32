#include "rtc.h"

static I2C_HandleTypeDef *rtc_i2c;

// DS3231 base I2C address is 0x68.
// STM32 HAL requires it to be shifted left by 1 bit for the R/W bit.
#define DS3231_ADDRESS (0x68 << 1)

// --- Helper Functions for BCD Conversion ---
static uint8_t DecToBcd(uint8_t val) {
    return (uint8_t)( (val / 10 * 16) + (val % 10) );
}

static uint8_t BcdToDec(uint8_t val) {
    return (uint8_t)( (val / 16 * 10) + (val % 16) );
}

void RTC_Init(I2C_HandleTypeDef *hi2c) {
    rtc_i2c = hi2c;
}

uint8_t RTC_GetTime(RTC_TimeTypeDef *time) {
    uint8_t reg = 0x00; // Start reading at register 0x00 (Seconds)
    uint8_t buffer[7];

    // Tell the RTC we want to read starting from register 0x00
    if (HAL_I2C_Master_Transmit(rtc_i2c, DS3231_ADDRESS, &reg, 1, 100) != HAL_OK) {
        return 0; // Hardware offline or error
    }

    // Read all 7 time/date registers in one continuous burst
    if (HAL_I2C_Master_Receive(rtc_i2c, DS3231_ADDRESS, buffer, 7, 100) != HAL_OK) {
        return 0;
    }

    // Convert raw BCD data back into human-readable decimal
    time->Seconds   = BcdToDec(buffer[0]);
    time->Minutes   = BcdToDec(buffer[1]);
    time->Hour      = BcdToDec(buffer[2]); // Assuming 24-hour mode
    time->DayOfWeek = BcdToDec(buffer[3]);
    time->Date      = BcdToDec(buffer[4]);
    time->Month     = BcdToDec(buffer[5] & 0x7F); // Mask out the century bit
    time->Year      = BcdToDec(buffer[6]);

    return 1; // Success
}

uint8_t RTC_SetTime(RTC_TimeTypeDef *time) {
    uint8_t buffer[8];

    buffer[0] = 0x00; // Start writing at register 0x00

    // Convert human-readable decimal into BCD for the RTC chip
    buffer[1] = DecToBcd(time->Seconds);
    buffer[2] = DecToBcd(time->Minutes);
    buffer[3] = DecToBcd(time->Hour); // Sets 24-hour mode automatically
    buffer[4] = DecToBcd(time->DayOfWeek);
    buffer[5] = DecToBcd(time->Date);
    buffer[6] = DecToBcd(time->Month);
    buffer[7] = DecToBcd(time->Year);

    // Blast all 8 bytes (Register Address + 7 Data Bytes) to the chip
    if (HAL_I2C_Master_Transmit(rtc_i2c, DS3231_ADDRESS, buffer, 8, 100) != HAL_OK) {
        return 0; // Error
    }

    return 1; // Success
}
