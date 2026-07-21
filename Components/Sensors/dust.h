#ifndef DUST_H
#define DUST_H

#include "stm32f1xx_hal.h"

// Initialize ADC buffer
void initDustSensor(void);

// Read dust concentration, scaled x10
// Returns 0 if error occurred
int16_t readDust(void);

// Expose raw data for the OLED Debug Page
extern uint16_t dust_raw_adc;
extern float dust_raw_voltage;

#endif /* DUST_H */
