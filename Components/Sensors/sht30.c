#include "sht30.h"

static I2C_HandleTypeDef *sht_i2c;
static uint8_t sht_addr;

void SHT30_Init(I2C_HandleTypeDef *hi2c) {
    sht_i2c = hi2c;

    // The SHT30 7-bit address is 0x44 when the AD pin is tied to GND.
    // The STM32 HAL requires the 7-bit address to be shifted left by 1 bit.
    sht_addr = 0x44 << 1;
}

uint8_t SHT30_Read(float *temperature, float *humidity) {
    // 0x2400 is the standard SHT3x command for: High repeatability, clock stretching disabled
    uint8_t cmd[2] = {0x24, 0x00};
    uint8_t buffer[6];

    // Send the 16-bit measurement command
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(sht_i2c, sht_addr, cmd, 2, 100);

    if (status != HAL_OK) {
        return 0; // Return an obvious error value if the sensor disconnects
    }

    // Wait for the measurement to complete
    // High repeatability mode requires a maximum of 15ms conversion time
    HAL_Delay(15);

    // Read the 6 bytes of data: Temp MSB, Temp LSB, Temp CRC, Hum MSB, Hum LSB, Hum CRC
    status = HAL_I2C_Master_Receive(sht_i2c, sht_addr, buffer, 6, 100);

    if (status != HAL_OK) {
        return 0;
    }

    // Combine the bytes for Temperature (The MSB is in buffer[0], LSB in buffer[1])
    uint16_t raw_temp = (buffer[0] << 8) | buffer[1];

    // Calculate actual Temperature in Celsius using the SHT3x datasheet formula
    *temperature = -45.0f + (175.0f * ((float)raw_temp / 65535.0f));

    // Combine the bytes for Humidity
    uint16_t raw_hum = (buffer[3] << 8) | buffer[4];

    // Calculate actual Humidity in % using the SHT3x datasheet formula
    *humidity = 100.0f * ((float)raw_hum / 65535.0f);

    return 1; // Success
}
