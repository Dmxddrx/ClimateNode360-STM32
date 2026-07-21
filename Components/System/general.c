#include "general.h"

// --- GLOBAL VARIABLES ---
uint8_t esp_is_ready = 0;
uint8_t wifi_is_connected = 0;
char current_ip[16] = "0.0.0.0";
char current_ssid[32] = "Not Connected";
char current_pc_ip[16] = "0.0.0.0";

extern uint8_t format_requested;


void Scan_I2C_Bus(I2C_HandleTypeDef *hi2c, char *buffer) {
    char hex[8];
    uint8_t devices_found = 0;
    buffer[0] = '\0';

    for (uint8_t i = 1; i < 128; i++) {
        if (HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(i << 1), 2, 10) == HAL_OK) {
            sprintf(hex, "0x%02X ", i);
            strcat(buffer, hex);
            devices_found++;
        }
    }
    if (devices_found == 0) {
        strcpy(buffer, "NONE");
    }
}

/* ═══════════════════════════════════════════════════════════════ */
/* TELEMETRY BROADCAST ENGINE                                      */
/* ═══════════════════════════════════════════════════════════════ */
static void send_telemetry_wifi(float temp, float hum, int16_t dust, uint16_t raw_adc, float raw_volt, const char* status) {
    if (!wifi_is_connected) return;

    static char tx_buf[256];

    /* Format a single JSON packet containing all environmental and debug data */
    snprintf(tx_buf, sizeof(tx_buf),
        "{\"type\":\"ENV\",\"temp\":%.1f,\"hum\":%.1f,\"dust\":%.1f,\"raw_adc\":%u,\"raw_volt\":%.2f,\"status\":\"%s\"}\n",
        temp, hum, (float)dust / 10.0f, raw_adc, raw_volt, status);

    WIFI_SendUDPData(tx_buf);
}

/* ═══════════════════════════════════════════════════════════════ */
/* NETWORK CONNECTION ROUTINE (Handles Fallback & UDP Binding)     */
/* ═══════════════════════════════════════════════════════════════ */
void Network_Connect_Routine(void) {
	OLED_ClearArea(0, 16, 128, 48); // Clear lower area for connection UI
    OLED_Print(0, 0, "NETWORK CONNECT");
    OLED_Update();

    const char* ssids[4]      = {WIFI_SSID_1, WIFI_SSID_2, WIFI_SSID_3, WIFI_SSID_4};
    const char* passes[4]     = {WIFI_PASS_1, WIFI_PASS_2, WIFI_PASS_3, WIFI_PASS_4};
    const char* target_ips[4] = {PC_IP_1, PC_IP_2, PC_IP_3, PC_IP_4};

    const char* active_pc_ip = NULL;
    wifi_is_connected = 0;

    for (int i = 0; i < 4; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Trying AP %d...     ", i + 1);
        OLED_Print(0, 16, buf);
        OLED_Update();

        if (WIFI_Connect((char*)ssids[i], (char*)passes[i])) {
            snprintf(buf, sizeof(buf), "AP %d Connected!    ", i + 1);
            OLED_Print(0, 26, buf);

            wifi_is_connected = 1;
            Buzzer_WifiConnected();
            LED_WifiConnected();
            active_pc_ip = target_ips[i];

            strncpy(current_ssid, ssids[i], sizeof(current_ssid) - 1);
            strncpy(current_pc_ip, target_ips[i], sizeof(current_pc_ip) - 1);
            break;
        } else {
            WIFI_Disconnect();
            HAL_Delay(500);
        }
    }

    if (wifi_is_connected && active_pc_ip != NULL) {
        if (WIFI_GetIP(current_ip)) {
            OLED_Print(0, 36, "IP: ");
            OLED_Print(24, 36, current_ip);
        }
        OLED_Update();

		// 1. Give the ESP TCP/IP stack 500ms to stabilize after getting its IP
		HAL_Delay(500);

		// 2. Force close any phantom sockets from previous resets
		WIFI_SendCommand("AT+CIPCLOSE\r\n");
		HAL_Delay(500);

        if (WIFI_StartUDP((char*)active_pc_ip, UDP_PORT)) {
            OLED_Print(0, 46, "UDP Ready!");
        } else {
            OLED_Print(0, 46, "Socket Failure");
            wifi_is_connected = 0;
            Buzzer_WifiConnected();
            LED_WifiDisconnected();
        }
        OLED_Update();
        HAL_Delay(2000);
    } else {
        OLED_Print(0, 36, "APs Failed. Solo Mode");
        OLED_Update();
        LED_HardwareError();
        HAL_Delay(1000);
    }
}



void General_Init(void) {
    // ---------------------------------------------------------
    // PAGE 1: HARDWARE DIAGNOSTICS
    // ---------------------------------------------------------

	// --- I2C CHECK ---
    OLED_Init(&hi2c2);
    HAL_Delay(100);


    char print_buf[150];
    char i2c1_res[128];
    char i2c2_res[128];

    Scan_I2C_Bus(&hi2c1, i2c1_res);
    Scan_I2C_Bus(&hi2c2, i2c2_res);

    OLED_Clear();
    OLED_Print(0, 0, "System Diagnostics");

    snprintf(print_buf, sizeof(print_buf), "I2C1: %s", i2c1_res);
    OLED_Print(0, 16, print_buf);

    snprintf(print_buf, sizeof(print_buf), "I2C2: %s", i2c2_res);
    OLED_Print(0, 26, print_buf);

	// Initialize Temperature Sensor, Dust sensor
    SHT30_Init(&hi2c1);
    initDustSensor();

		float temp = 0.0f, hum = 0.0f;
		SHT30_Read(&temp, &hum);
    int16_t dust = readDust();

		char temp_str[32];
		snprintf(temp_str, sizeof(temp_str), "%.1fC %.1f%%", temp, hum);
		OLED_Print(60, 16, temp_str);

		char dust_str[32];
		snprintf(dust_str, sizeof(dust_str), "%.1fug", (float)dust / 10.0f);
		OLED_Print(60, 26, dust_str); // Prints next to I2C2

		Buzzer_BootSound();
		LED_Boot();

    // --- SPI / SD CARD CHECK ---
    if (HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_READY) {

		// Use our new wrapper function instead of manual f_mount
		if (SD_Init()) {
			OLED_Print(0, 36, "SPI/SD: OK");
		} else {
			OLED_Print(0, 36, "SPI/SD: MOUNT ERR");
		}

	} else {
		OLED_Print(0, 36, "SPI/SD: OFFLINE");
	}

    // --- USART2 CHECK ---
	if (HAL_UART_GetState(&huart2) == HAL_UART_STATE_READY) {
		OLED_Print(0, 46, "USART2: OK");
	} else {
		OLED_Print(0, 46, "USART2: ERROR");
	}

    OLED_Update();
    HAL_Delay(4000); // Hold Hardware Diagnostics for 4 seconds

    // ---------------------------------------------------------
    // PAGE 2: NETWORK DIAGNOSTICS & DYNAMIC AP FAILOVER
    // ---------------------------------------------------------
    OLED_Clear();
    OLED_Print(0, 0, "NETWORK INIT");
    OLED_Print(0, 16, "Booting ESP-01S");
    OLED_Update();

    if (WIFI_Init()) {
        OLED_Print(80, 16, "OK");
        esp_is_ready = 1;
        OLED_Update();

        Network_Connect_Routine();

    } else {
        OLED_Print(80, 16, "FAIL");
        esp_is_ready = 0;
        OLED_Update();
        HAL_Delay(2000);

        // ---------------------------------------------------------
		// PAGE 3: SHOW INITIAL SENSOR PAGE & WAIT BEFORE MAIN LOOP
		// ---------------------------------------------------------
        OLED_GUI_DrawSensorPage(temp, hum, dust);
        OLED_Update();
        HAL_Delay(4000);
        return;
    }

    // ---------------------------------------------------------
    // PAGE 3: SHOW INITIAL SENSOR PAGE & WAIT BEFORE MAIN LOOP
    // ---------------------------------------------------------
    OLED_GUI_DrawSensorPage(temp, hum, dust);
    OLED_Update();
    HAL_Delay(1000);
}

// ---------------------------------------------------------
// MAIN NON-BLOCKING LOOP
// ---------------------------------------------------------
void General_Run(void) {
    // 1. Pet the hardware watchdog continuously
    HAL_IWDG_Refresh(&hiwdg);
    BTNS_Update();

    // Independent Loop Timers
    static uint32_t last_ui_tick = 0;
    static uint32_t last_dust_tick = 0;
    static uint32_t last_sht30_tick = 0;
    static uint32_t last_log_tick = 0;
    static uint32_t last_telemetry_tick = 0; // <-- NEW: 60-second telemetry timer

    // Shared Sensor Data
    static uint8_t current_page = 0;
    static float current_temp = 0.0f;
    static float current_hum = 0.0f;
    static int16_t current_dust = 0;
    static uint8_t sensor_ok = 1;

    static uint8_t first_run = 1;
    uint32_t now = HAL_GetTick();

    // --- NEW: SD CARD FORMAT INTERRUPT ---
	if (format_requested) {
		format_requested = 0; // Clear the flag immediately

		Buzzer_LongPressAck(); // Acknowledge the long press

		OLED_Clear();
		OLED_Print(0, 0, "FORMATTING SD...");
		OLED_Print(0, 16, "Please wait.");
		OLED_Update();

		if (SD_Format()) {
			OLED_Print(0, 36, "FORMAT SUCCESS!");
			Buzzer_FormatSuccess(); // Success chime
			LED_FormatSuccess();
		} else {
			OLED_Print(0, 36, "FORMAT FAILED!");
			Buzzer_FormatFail(); // Error drone
			LED_FormatFail();
		}
		OLED_Update();

		HAL_Delay(2000); // Hold the result on screen for 2 seconds

		// Force the display to instantly redraw the current page
		        if (current_page == 0) {
		            OLED_GUI_DrawSensorPage(current_temp, current_hum, current_dust);
		        } else if (current_page == 1) {
		            render_wifi_page();
		        } else if (current_page == 2) {
		            OLED_GUI_DrawDustDebugPage(dust_raw_adc, dust_raw_voltage, current_dust);
		        } else if (current_page == 3) {
		            OLED_GUI_DrawWeatherPage(current_temp, current_hum, current_dust);
		        } else if (current_page == 4) {
		            OLED_GUI_DrawDiagnosticsPage();
		        } else if (current_page == 5) {
		            OLED_GUI_DrawLogPage();
		        }
		        OLED_Update();
	}

    // Force an immediate poll of everything on first boot
    if (first_run) {
        last_ui_tick = now - 50;
        last_dust_tick = now - 500;
        last_sht30_tick = now - 2000;
        last_log_tick = now - 15000;
        last_telemetry_tick = now - 60000; // Force immediate broadcast on boot
        first_run = 0;
    }

    // ---------------------------------------------------------
    // LOOP 1: DUST SENSOR & FAST OLED UPDATE (Every 500ms)
    // ---------------------------------------------------------
    if (now - last_dust_tick >= 500) {
        last_dust_tick = now;

        current_dust = readDust();

        // Auto-refresh the screens that show dust data instantly
        if (current_page == 0) {
            OLED_GUI_DrawSensorPage(current_temp, current_hum, current_dust);
            OLED_Update();
        } else if (current_page == 2) {
            OLED_GUI_DrawDustDebugPage(dust_raw_adc, dust_raw_voltage, current_dust);
            OLED_Update();
        } else if (current_page == 3) {
            OLED_GUI_DrawWeatherPage(current_temp, current_hum, current_dust);
            OLED_Update();
        }

        // Notice we removed the Wi-Fi transmission from here!
    }

    // ---------------------------------------------------------
    // LOOP 2: SHT30 CLIMATE (Every 2 Seconds)
    // ---------------------------------------------------------
    if (now - last_sht30_tick >= 2000) {
        last_sht30_tick = now;

        sensor_ok = SHT30_Read(&current_temp, &current_hum);

        // Auto-refresh the Main Sensor screen
        if (current_page == 0) {
            OLED_GUI_DrawSensorPage(current_temp, current_hum, current_dust);
            OLED_Update();
        } else if (current_page == 3) {
            OLED_GUI_DrawWeatherPage(current_temp, current_hum, current_dust);
            OLED_Update();
        }
    }

    // ---------------------------------------------------------
    // LOOP 3: SD LOGGING & WI-FI VERIFICATION (Every 15 Seconds)
    // ---------------------------------------------------------
    if (now - last_log_tick >= 15000) {
        last_log_tick = now;

        const char* current_status = sensor_ok ? "OK" : "ERROR";

        // --- SD CARD AUTO-REMOUNT ---
		// If the card was removed or failed at boot, try to initialize it again
		if (!SD_IsReady()) {
			SD_Init();

			// Play a quick chime so you know it successfully hot-swapped!
			if (SD_IsReady()) {
				Buzzer_SD_Inserted();
			}
		}

		// --- AUTO-REFRESH DIAGNOSTICS PAGE LIVE ---
		if (current_page == 4) {
			OLED_GUI_DrawDiagnosticsPage();
			OLED_Update();
		}


        // 2. Safely verify the router connection
        if (wifi_is_connected) {
            if (!WIFI_IsConnected()) {
                // Drop detected! Flag it offline.
                wifi_is_connected = 0;
                Buzzer_WifiDisconnected();
                LED_WifiDisconnected();
            }
        }

        // 3. INSTANT RECONNECT
        if (!wifi_is_connected && esp_is_ready) {
            Network_Connect_Routine();

            // Restore the GUI after reconnecting
            if (current_page == 0) {
                OLED_GUI_DrawSensorPage(current_temp, current_hum, current_dust);
            } else if (current_page == 1) {
                render_wifi_page();
            } else if (current_page == 2) {
                OLED_GUI_DrawDustDebugPage(dust_raw_adc, dust_raw_voltage, current_dust);
            } else if (current_page == 3) {
                OLED_GUI_DrawWeatherPage(current_temp, current_hum, current_dust);
            } else if (current_page == 4) {         // <--- Add this block
				OLED_GUI_DrawDiagnosticsPage();
			} else if (current_page == 5) {
	            OLED_GUI_DrawLogPage();
	        }
            OLED_Update();
        }
    }

    // ---------------------------------------------------------
    // LOOP 4: BUTTON UI (Every 50ms)
    // ---------------------------------------------------------
    if (now - last_ui_tick >= 50) {
        last_ui_tick = now;

        if (BTNS_Get_OLEDPage() == BTN_PRESSED) {
            current_page++;
            if (current_page > 5) current_page = 0;

            if (current_page == 0) {
                OLED_GUI_DrawSensorPage(current_temp, current_hum, current_dust);
            } else if (current_page == 1) {
                render_wifi_page();
            } else if (current_page == 2) {
                OLED_GUI_DrawDustDebugPage(dust_raw_adc, dust_raw_voltage, current_dust);
            } else if (current_page == 3) {
                OLED_GUI_DrawWeatherPage(current_temp, current_hum, current_dust);
            } else if (current_page == 4) {         // <--- Add this block
				OLED_GUI_DrawDiagnosticsPage();
			} else if (current_page == 5) {
	            OLED_GUI_DrawLogPage();
	        }
            OLED_Update();
        }
    }

    // ---------------------------------------------------------
    // LOOP 5: DMA TELEMETRY BROADCAST (Every 60 Seconds)
    // ---------------------------------------------------------
    if (now - last_telemetry_tick >= 60000) {
        last_telemetry_tick = now;

        const char* current_status = sensor_ok ? "OK" : "ERROR";

		// =========================================================
		// TASK A: SD CARD LOGGING
		// =========================================================

		// Track the state BEFORE we attempt to write
		uint8_t sd_was_ready = SD_IsReady();

		// 1. Log to CSV (SD Card Chip)
		SD_LogData(current_temp, current_hum, current_dust, current_status);

		// 2. Play dismount sound if the write attempt just failed
		if (sd_was_ready && !SD_IsReady()) {
			Buzzer_SD_Removed(); // Play the dismount warning!
		}

		// 3. AUTO-REFRESH LOG PAGE LIVE
		if (current_page == 5) {
			OLED_GUI_DrawLogPage();
			OLED_Update();
		}


		// =========================================================
		// TASK B: WI-FI UPLOAD
		// =========================================================
        if (wifi_is_connected) {
            const char* current_status = sensor_ok ? "OK" : "ERROR";
            send_telemetry_wifi(current_temp, current_hum, current_dust, dust_raw_adc, dust_raw_voltage, current_status);
            LED_UploadSuccess();
        } else {
        	LED_UploadFail();
        }
    }
}
