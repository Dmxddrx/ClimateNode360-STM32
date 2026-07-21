#include "oledGUI.h"

// 24x24 Weather-App Style Thermometer Icon
const uint8_t icon_temp_24x24[72] = {
    0x00, 0x18, 0x00, //       **
    0x00, 0x24, 0x00, //      *  *
    0x00, 0x25, 0x00, //      *  * *   (Tick Mark)
    0x00, 0x24, 0x00, //      *  *
    0x00, 0x3C, 0x00, //      ****     (Mercury Level)
    0x00, 0x3D, 0x00, //      **** *   (Tick Mark)
    0x00, 0x3C, 0x00, //      ****
    0x00, 0x3C, 0x00, //      ****
    0x00, 0x3D, 0x00, //      **** *   (Tick Mark)
    0x00, 0x3C, 0x00, //      ****
    0x00, 0x3C, 0x00, //      ****
    0x00, 0x3D, 0x00, //      **** *   (Tick Mark)
    0x00, 0x3C, 0x00, //      ****
    0x00, 0x3C, 0x00, //      ****
    0x00, 0x5A, 0x00, //     * ** *    (Bulb Widens)
    0x00, 0x99, 0x00, //    *  **  *
    0x01, 0x3C, 0x80, //   *  ****  *
    0x02, 0x7E, 0x40, //  *  ******  * (Bulb Filled)
    0x02, 0x7E, 0x40, //  *  ******  *
    0x02, 0x7E, 0x40, //  *  ******  *
    0x01, 0x3C, 0x80, //   *  ****  *
    0x00, 0x99, 0x00, //    *  **  *
    0x00, 0x7E, 0x00, //     ******
    0x00, 0x00, 0x00  //
};

void OLED_GUI_DrawSensorPage(float temperature, float humidity, int16_t dust) {
    char temp_str[32];
    char hum_str[32];
    char dust_str[32];

    OLED_Clear();
    OLED_Print(0, 0, "ClimateNode 360");

    if (temperature <= -40.0f) {
        OLED_Print(0, 16, "Sensor: ERROR");
    } else {
        snprintf(temp_str, sizeof(temp_str), "Temp: %.1f C", temperature);
        OLED_Print(0, 16, temp_str);

        snprintf(hum_str, sizeof(hum_str), "Hum:  %.1f %%", humidity);
        OLED_Print(0, 26, hum_str);

		snprintf(dust_str, sizeof(dust_str), "Dust: %.1f ug", (float)dust / 10.0f);
		OLED_Print(0, 36, dust_str);
    }
}

/* ═══════════════════════════════════════════════════════════════ */
/* WI-FI / NETWORK PAGE (2)                                        */
/* ═══════════════════════════════════════════════════════════════ */
void render_wifi_page(void) {
    OLED_Clear();
    OLED_Print(0, 0, "NETWORK");

    //OLED_DrawOutline(0, 16, 96, 10);

    OLED_Print(0, 16, "ESP-01S");
    if (esp_is_ready) {
        OLED_Print(0, 26, "READY");
    } else {
        OLED_Print(0, 26, "ERROR");
    }

    OLED_Print(50, 16, "Wi-Fi");
    if (wifi_is_connected) {
        OLED_Print(50, 26, "ONLINE");
        OLED_Print(50, 0, current_ip);

		OLED_Print(0, 38, current_ssid);
		OLED_Print(0, 48, current_pc_ip);

    } else {
        OLED_Print(50, 26, "OFFLINE");
        OLED_Print(50, 0, "Not Assigned");

		char last_ap_buf[40];
		snprintf(last_ap_buf, sizeof(last_ap_buf), "Last: %s", current_ssid);
		OLED_Print(0, 38, last_ap_buf);
		OLED_Print(0, 48, "IP: Disconnected");

    }
}

void OLED_GUI_DrawDustDebugPage(uint16_t raw_adc, float voltage, int16_t dust) {
    char buf[32];

    OLED_Clear();
    OLED_Print(0, 0, "DUST DEBUG (Raw)");

    // Print Raw ADC (0 - 4095)
    snprintf(buf, sizeof(buf), "ADC Val: %u", raw_adc);
    OLED_Print(0, 16, buf);

    // Print Raw Calculated Voltage
    snprintf(buf, sizeof(buf), "Voltage: %.2f V", voltage);
    OLED_Print(0, 26, buf);

    // Print Final Scaled Density
    snprintf(buf, sizeof(buf), "Density: %.1f ug", (float)dust / 10.0f);
    OLED_Print(0, 36, buf);
}

/* ═══════════════════════════════════════════════════════════════ */
/* WEATHER APP PAGE (PAGE 3)                                       */
/* ═══════════════════════════════════════════════════════════════ */
void OLED_GUI_DrawWeatherPage(float temperature, float humidity, int16_t dust) {
    char temp_str[32];

    OLED_Clear();

    // Draw the new 24x24 Thermometer Icon at X=0, Y=5
    OLED_DrawBitmap(0, 5, icon_temp_24x24, 24, 24);

    // Draw the text next to it (Standard font for now, 11x18 coming next!)
    if (temperature <= -40.0f) {
        OLED_Print(30, 15, "ERROR");
    } else {
        snprintf(temp_str, sizeof(temp_str), "%.1f C", temperature);
        OLED_Print(30, 15, temp_str);
    }
}

/* ═══════════════════════════════════════════════════════════════ */
/* SYSTEM DIAGNOSTICS PAGE (PAGE 4)                                */
/* ═══════════════════════════════════════════════════════════════ */
void OLED_GUI_DrawDiagnosticsPage(void) {
    OLED_Clear();
    OLED_Print(0, 0, "System Diagnostics");

    // --- SPI / SD CARD CHECK ---
    if (HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_READY) {
        if (SD_IsReady()) {
            OLED_Print(0, 16, "SPI/SD: OK");
        } else {
            OLED_Print(0, 16, "SPI/SD: MOUNT ERR");
        }
    } else {
        OLED_Print(0, 16, "SPI/SD: OFFLINE");
    }

    // --- USART2 (Wi-Fi) CHECK ---
    if (HAL_UART_GetState(&huart2) == HAL_UART_STATE_READY) {
        OLED_Print(0, 26, "USART2: OK");
    } else {
        OLED_Print(0, 26, "USART2: ERROR");
    }

    // --- I2C CHECK ---
    if (HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_READY) {
        OLED_Print(0, 36, "I2C1:   OK");
    } else {
        OLED_Print(0, 36, "I2C1:   BUSY/ERR");
    }
}

/* ═══════════════════════════════════════════════════════════════ */
/* SD CARD LOGGING PAGE (PAGE 5)                                   */
/* ═══════════════════════════════════════════════════════════════ */
void OLED_GUI_DrawLogPage(void) {
    OLED_Clear();
    OLED_Print(0, 0, "SD Card Log Data");

    if (!SD_IsReady()) {
        OLED_Print(0, 20, "Status: OFFLINE");
        OLED_Print(0, 32, "Check SD Card!");
        return;
    }

    uint32_t rows;
    char last_data[32];
    SD_GetLogStats(&rows, last_data);

    char buf[32];
    OLED_Print(0, 16, "File: data.csv");

    snprintf(buf, sizeof(buf), "Rows: %lu", rows);
    OLED_Print(0, 28, buf);

    OLED_Print(0, 42, "Latest Entry:");
    OLED_Print(0, 52, last_data);
}
