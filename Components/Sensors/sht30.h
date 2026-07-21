#ifndef SHT30_H
#define SHT30_H

#include "stm32f1xx_hal.h" //[cite: 5]

// Initialize the SHT30 sensor
// Address is configured internally for AD connected to GND
void SHT30_Init(I2C_HandleTypeDef *hi2c);

// Trigger a measurement and read both temperature and humidity
// Returns 1 on success, 0 on failure
uint8_t SHT30_Read(float *temperature, float *humidity);

#endif /* SHT30_H */
