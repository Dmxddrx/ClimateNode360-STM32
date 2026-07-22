#include "general.h"

// --- GLOBAL VARIABLES ---
uint8_t esp_is_ready = 0;
uint8_t wifi_is_connected = 0;
uint8_t tcp_is_connected = 0;
uint8_t ntp_sync_status = 0;

char current_ip[16] = "0.0.0.0";
char current_ssid[32] = "Not Connected";
char current_pc_ip[16] = "0.0.0.0";

extern uint8_t format_requested;

static uint8_t current_ap_index = 0;
static uint32_t last_wifi_attempt_tick = 0;
static uint32_t wifi_retry_delay = 90000;


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
static int8_t send_telemetry_wifi(float temp, float hum, int16_t dust, uint16_t raw_adc, float raw_volt, const char* status) {
    if (!tcp_is_connected) return 0;

    static char tx_buf[256];

    snprintf(tx_buf, sizeof(tx_buf),
        "{\"type\":\"ENV\",\"temp\":%.1f,\"hum\":%.1f,\"dust\":%.1f,\"raw_adc\":%u,\"raw_volt\":%.2f,\"status\":\"%s\"}\n",
        temp, hum, (float)dust / 10.0f, raw_adc, raw_volt, status);

    return WIFI_SendTCPData(tx_buf);
}

/* ═══════════════════════════════════════════════════════════════ */
/* STAGGERED NETWORK CONNECTION (Dynamic Fast/Slow Retry)          */
/* ═══════════════════════════════════════════════════════════════ */
void Try_Next_WiFi(void) {
    const char* ssids[4]      = {WIFI_SSID_1, WIFI_SSID_2, WIFI_SSID_3, WIFI_SSID_4};
    const char* passes[4]     = {WIFI_PASS_1, WIFI_PASS_2, WIFI_PASS_3, WIFI_PASS_4};
    const char* target_ips[4] = {PC_IP_1, PC_IP_2, PC_IP_3, PC_IP_4};

    OLED_ClearArea(0, 16, 128, 48);
    char buf[32];
    snprintf(buf, sizeof(buf), "Trying AP %d...", current_ap_index + 1);
    OLED_Print(0, 16, buf);
    OLED_Update();

    // Try to connect to just the current index
    if (WIFI_Connect(ssids[current_ap_index], passes[current_ap_index])) {

        // --- Wi-Fi Connected Successfully! ---
        wifi_is_connected = 1;
        Buzzer_WifiConnected();
        LED_WifiConnected();

        strncpy(current_ssid, ssids[current_ap_index], sizeof(current_ssid) - 1);
        strncpy(current_pc_ip, target_ips[current_ap_index], sizeof(current_pc_ip) - 1);

        if (WIFI_GetIP(current_ip)) {
            OLED_Print(0, 26, "IP: ");
            OLED_Print(24, 26, current_ip);
        }
        OLED_Update();
        HAL_Delay(500);

        WIFI_SendCommand("AT+CIPCLOSE\r\n"); // Drop dead sockets
        HAL_Delay(500);

        // ========================================================
		// 1. SYNC TIME FIRST (Before opening TCP)
		// ========================================================
		if (wifi_is_connected) {
			OLED_Print(0, 46, "Syncing Time...");
			OLED_Update();
			RTC_TimeTypeDef net_time;
			if (WIFI_GetNTPTime(&net_time)) {
				RTC_SetTime(&net_time);
				ntp_sync_status = 1;
				OLED_Print(0, 46, "Time Synced!   ");
			} else {
				ntp_sync_status = 0;
				OLED_Print(0, 46, "Time Sync Fail ");
				LED_NtpFailed();
			}
			OLED_Update();
		}
		HAL_Delay(1000); // Give the ESP a breather

		// ========================================================
		// 2. CONNECT TCP LAST (So it doesn't time out while idle)
		// ========================================================
		WIFI_SendCommand("AT+CIPCLOSE\r\n"); // Drop dead sockets
		HAL_Delay(500);

		if (WIFI_StartTCP(current_pc_ip, TCP_PORT)) {
			OLED_Print(0, 36, "TCP Ready!");
			tcp_is_connected = 1;
			wifi_retry_delay = 90000;
			LED_TcpConnected();
		} else {
			OLED_Print(0, 36, "Socket Failure");
			tcp_is_connected = 0;
			wifi_retry_delay = 90000;
			LED_TcpFailed();
		}
		OLED_Update();
		HAL_Delay(2000);

    } else {
        // --- NEW: The AP itself failed! ---
    	WIFI_Disconnect();
		wifi_is_connected = 0;
		tcp_is_connected = 0; // Safety wipe
		OLED_Print(0, 26, "AP Failed.");
		OLED_Update();
		LED_HardwareError();
		HAL_Delay(1000);

        // Wait 5 full minutes before attempting the next router
        wifi_retry_delay = 300000;

        // Move to the next AP index
        current_ap_index++;
        if (current_ap_index >= 4) current_ap_index = 0;
    }
}


/* ═══════════════════════════════════════════════════════════════ */
/* BULK SD CARD UPLOAD ROUTINE (FAIL-SAFE)                                */
/* ═══════════════════════════════════════════════════════════════ */
void Upload_And_Clear_SD(void) {
    if (!SD_IsReady()) return;

    uint32_t rows;
    char temp_str[32];
    SD_GetLogStats(&rows, temp_str);

    if (rows == 0) return; // The card is empty, skip upload!

    OLED_ClearArea(0, 16, 128, 48);
    OLED_Print(0, 16, "Uploading SD...");
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu Rows left", rows);
    OLED_Print(0, 26, buf);
    OLED_Update();

    FIL file;
    if (f_open(&file, "data.csv", FA_READ | FA_OPEN_EXISTING) == FR_OK) {
        char line[128]; // Buffer to hold one row of CSV data
        uint8_t has_data = 0;
        uint8_t upload_success = 1;

        // Read the file line-by-line until we hit the end
        while (f_gets(line, sizeof(line), &file)) {
            has_data = 1;
            HAL_IWDG_Refresh(&hiwdg); // Pet the dog during long uploads

            // --- NEW: If a single line fails to send, abort immediately! ---
			if (!WIFI_SendTCPData(line)) {
				upload_success = 0; // Mark the entire upload as failed
				break;              // Stop reading the file
			}

            HAL_Delay(20); // 20ms breather so we don't overwhelm the ESP8266 RAM
        }
        f_close(&file);

        // --- NEW: Only wipe the file if EVERY line was acknowledged by the server ---
		if (has_data && upload_success) {
			SD_ClearLog(); // Wipe the file!
			OLED_Print(0, 46, "Upload Complete!");
			OLED_Update();
			HAL_Delay(1500);
		} else if (!upload_success) {
            OLED_Print(0, 46, "Upload FAILED!  ");
            OLED_Update();
            LED_HardwareError();
            HAL_Delay(1500);

            //tcp_is_connected = 0;

            // --- NEW: Trigger a fast retry in 15 seconds ---
			wifi_retry_delay = 15000;
        }
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
    RTC_Init(&hi2c1);
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

        Try_Next_WiFi();

        if (wifi_is_connected) {
        	HAL_Delay(1000);
			Upload_And_Clear_SD();
		}

    } else {
        OLED_Print(80, 16, "FAIL");
        esp_is_ready = 0;
        OLED_Update();
        HAL_Delay(2000);

        // ---------------------------------------------------------
		// PAGE 3: SHOW INITIAL SENSOR PAGE & WAIT BEFORE MAIN LOOP
		// ---------------------------------------------------------
        OLED_GUI_DrawTimePage();
        OLED_Update();
        HAL_Delay(4000);
        return;
    }

    // ---------------------------------------------------------
    // PAGE 3: SHOW INITIAL SENSOR PAGE & WAIT BEFORE MAIN LOOP
    // ---------------------------------------------------------
    OLED_GUI_DrawTimePage();
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
		        } else if (current_page == 6) {     // <--- ADD THIS
		            OLED_GUI_DrawTimePage();
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
        } else if (current_page == 6) {     // <--- ADD THIS
            OLED_GUI_DrawTimePage();
            OLED_Update();
        }

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
	// LOOP 3: HARDWARE VERIFICATION & RECONNECT (Every 15 Seconds)
	// ---------------------------------------------------------
	if (now - last_log_tick >= 15000) {
		last_log_tick = now;

		// --- SD CARD AUTO-REMOUNT ---
		if (!SD_IsReady()) {
			SD_Init();
			if (SD_IsReady()) Buzzer_SD_Inserted();
		}

		// --- AUTO-REFRESH DIAGNOSTICS PAGE LIVE ---
		if (current_page == 4) {
			OLED_GUI_DrawDiagnosticsPage();
			OLED_Update();
		}

		// 2. Safely verify the router connection
		if (wifi_is_connected) {
			if (!WIFI_IsConnected()) {
				// Router Drop detected! Flag BOTH offline.
				wifi_is_connected = 0;
				tcp_is_connected = 0;
				Buzzer_WifiDisconnected();
				LED_WifiDisconnected();
				last_wifi_attempt_tick = HAL_GetTick();
			}
			else if (!tcp_is_connected) {
				// Wi-Fi is perfectly fine, but TCP is offline!
				// Retry TCP socket silently every 90 seconds.
				if (now - last_wifi_attempt_tick >= wifi_retry_delay) {
					last_wifi_attempt_tick = HAL_GetTick();

					LED_TcpRetry();

					if (WIFI_StartTCP(current_pc_ip, TCP_PORT)) {
						tcp_is_connected = 1;
						LED_TcpConnected();
						Upload_And_Clear_SD(); // Upload the backlog!
					} else {
						LED_TcpFailed();
					}

					// Refresh GUI if user is watching the Network page
					if (current_page == 1) {
						render_wifi_page();
						OLED_Update();
					}
				}
			}
		} else if (esp_is_ready) {
			// 3. FULL AP RECONNECT (Every 90 seconds)
			if (now - last_wifi_attempt_tick >= wifi_retry_delay) {

				Try_Next_WiFi(); // Tries exactly 1 AP, then exits

				last_wifi_attempt_tick = HAL_GetTick();

				if (wifi_is_connected && tcp_is_connected) {
					Upload_And_Clear_SD(); // Dump the SD card payload!
				}

				// Restore the GUI after reconnect attempt finishes
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
				} else if (current_page == 6) {
					OLED_GUI_DrawTimePage();
				}
				OLED_Update();
			}
		}
	}

    // ---------------------------------------------------------
    // LOOP 4: BUTTON UI (Every 50ms)
    // ---------------------------------------------------------
    if (now - last_ui_tick >= 50) {
        last_ui_tick = now;

        if (BTNS_Get_OLEDPage() == BTN_PRESSED) {
            current_page++;
            if (current_page > 6) current_page = 0;

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
	        } else if (current_page == 6) {     // <--- ADD THIS
	            OLED_GUI_DrawTimePage();
	        }
            OLED_Update();
        }
    }

    // ---------------------------------------------------------
	// LOOP 5: SD LOGGING & TELEMETRY BROADCAST (Every 60 Seconds)
	// ---------------------------------------------------------
	if (now - last_telemetry_tick >= 60000) {
		last_telemetry_tick = now;

		// --- NEW: Silent Background Time Sync Retry ---
		// If we are online but the time failed earlier, try asking again!
		if (wifi_is_connected && !ntp_sync_status) {
			RTC_TimeTypeDef net_time;
			if (WIFI_GetNTPTime(&net_time)) {
				RTC_SetTime(&net_time);
				ntp_sync_status = 1; // Locked in!

				// Instantly refresh the screen if the user is looking at the Time page
				if (current_page == 6) {
					OLED_GUI_DrawTimePage();
					OLED_Update();
				}
			}
		}
		// ----------------------------------------------

		const char* current_status = sensor_ok ? "OK" : "ERROR";

		// If either the Router OR the Windows App is offline, log to SD
		if (!wifi_is_connected || !tcp_is_connected) {
			// =========================================================
			// TASK A: OFFLINE - STORE DATA TO SD CARD
			// =========================================================
			uint8_t sd_was_ready = SD_IsReady();

			SD_LogData(current_temp, current_hum, current_dust, current_status);

			if (sd_was_ready && !SD_IsReady()) {
				Buzzer_SD_Removed();
			}

			if (current_page == 5) {
				OLED_GUI_DrawLogPage();
				OLED_Update();
			}
		} else {
			// =========================================================
			// TASK B: ONLINE - SEND DATA LIVE VIA TCP
			// =========================================================
			if (send_telemetry_wifi(current_temp, current_hum, current_dust, dust_raw_adc, dust_raw_voltage, current_status)) {
				LED_UploadSuccess();
			} else {
				// If it fails mid-send, mark TCP offline instantly
				tcp_is_connected = 0;
				LED_UploadFail();

				// --- CRITICAL DATA RETENTION ---
				// Save the data that just failed to transmit!
				SD_LogData(current_temp, current_hum, current_dust, current_status);
			}
		}
	}
}
