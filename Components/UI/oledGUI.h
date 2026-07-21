#ifndef OLEDGUI_H
#define OLEDGUI_H

#include "oled.h"
#include "general.h"
#include <stdio.h>

// Function Prototypes
void OLED_GUI_DrawSensorPage(float temperature, float humidity, int16_t dust);
void OLED_GUI_DrawDustDebugPage(uint16_t raw_adc, float voltage, int16_t dust);
void OLED_GUI_DrawWeatherPage(float temperature, float humidity, int16_t dust);
void render_wifi_page(void);
void OLED_GUI_DrawDiagnosticsPage(void);
void OLED_GUI_DrawLogPage(void);
void OLED_GUI_DrawTimePage(void);

#endif /* OLEDGUI_H */
