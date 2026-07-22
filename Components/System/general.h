#ifndef GENERAL_H
#define GENERAL_H

/* =================================================================================
 * PROJECT: ClimateNode_360
 * DESCRIPTION: Advanced Wireless Environmental Monitoring Mesh Node
 *
 * CORE ARCHITECTURE & SYSTEM FEATURES:
 * - Non-Blocking State Machine: Completely timer-driven multi-loop architecture
 *   that prevents UI freezing during network drops or sensor reads.
 * - Hardware Watchdog (IWDG): Continuous background petting to guarantee the
 *   STM32 recovers from severe hardware lockups or electrical glitches.
 *
 * SENSORS & DATA PROCESSING:
 * - SHT30 Climate Sensor (I2C): Precision Temperature (°C) and Humidity (%) polling.
 * - Sharp Optical Dust Sensor (ADC): Precise microsecond polling with an advanced
 *   Exponential Moving Average (EMA) filter to prevent voltage spikes.
 *
 * NETWORK & TELEMETRY (ESP-01S AT-COMMAND ENGINE):
 * - Dynamic AP Failover: Stores up to 4 backup Wi-Fi routers/hotspots.
 * - Staggered Reconnect: 90-second "Quick Retry" for the current router, cascading
 *   into a 5-minute "Slow Search" across backup routers to save power.
 * - Decoupled TCP/Wi-Fi Tracking: Tracks the Wi-Fi connection and the C# App TCP
 *   socket independently. If the PC app closes, Wi-Fi stays connected and
 *   silently knocks on the TCP port every 90 seconds.
 * - Live JSON Telemetry: Pushes live environmental and raw debug data to the C# app.
 *
 * OFFLINE RESILIENCE & SD STORAGE (FATFS/SPI):
 * - Offline Auto-Logging: Drops to SD card CSV logging if Wi-Fi or TCP goes offline.
 * - Auto Bulk-Upload: The moment TCP reconnects, the system reads the CSV backlog
 *   and dumps all missing data to the C# Database line-by-line.
 * - Auto-Clear & Verify: Verifies TCP ACK for every row before wiping the SD card.
 * - Hot-Swap Detection: Safely detects if the SD card is removed and re-mounts it.
 * - Hardware Format: Holding the touch button for 3 seconds formats the drive (FAT32).
 *
 * TIMEKEEPING & NTP (DS3231 RTC):
 * - Mobile Hotspot NTP Parser: Custom Universal AT-Parser that ignores invisible
 *   carriage returns, detects network "Lag", and utilizes time.google.com.
 * - Local Offset Calculation: Natively calculates Sri Lanka's +05:30 offset directly
 *   on the STM32 to bypass old ESP8266 "5.5" firmware crash bugs.
 * - Background Sync: Silently retries time sync in LOOP 5 if it failed during boot.
 *
 * USER INTERFACE (0.96" YELLOW/BLUE OLED):
 * - Gap-Safe Rendering: Y-coordinates strictly mapped (16, 26, 36, etc.) to prevent
 *   pixels from falling into the physical gap between the yellow and blue zones.
 * - Capacitive Touch Paging: 300ms hardware debounce/cooldown for clean paging.
 * - 7 Dynamic GUI Pages:
 *   1. Main Sensor View (Temp, Hum, Dust)
 *   2. Network Status (ESP Status, SSID, Local IP, PC IP, TCP Socket State)
 *   3. Dust Debug (Raw ADC, Raw Voltage, Scaled Density)
 *   4. Weather UI (Custom 24x24 Thermometer Hex Bitmap)
 *   5. System Diagnostics (SPI, USART2, I2C Bus Live Status)
 *   6. SD Card Log (File Name, Pending Rows, Latest Data Preview)
 *   7. RTC Date & Time (Live Clock, Day of Week, NTP Sync Debug Output)
 *
 * AUDIO / VISUAL FEEDBACK:
 * - PWM Hardware Buzzer: Distinct melodic profiles for Boot, Wi-Fi Connect/Drop,
 *   SD Card Insert/Remove, Long-Press ACK, and Format Success/Fail.
 * - Common-Anode RGB LED: Non-blocking background animations for Connecting,
 *   TCP Sync, NTP Warnings, Upload Success/Fail, and Hardware Interventions.
 * =================================================================================
 */

#include "main.h"
#include "oled.h"
#include "fatfs.h"
#include "oledGUI.h"
#include "wifi.h"
#include "sd.h"
#include "btns.h"
#include "sht30.h"
#include "dust.h"
#include "buzzer.h"
#include "led.h"
#include "rtc.h"

#include <stdio.h>
#include <string.h>


extern IWDG_HandleTypeDef hiwdg;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern SPI_HandleTypeDef hspi1;

// --- NETWORK CONFIGURATIONS ---
//#define UDP_PORT    8080
#define TCP_PORT    8080

#define WIFI_SSID_2 "ENTGRA 2.5G"
#define WIFI_PASS_2 "Entgra@110"
#define PC_IP_2     "192.168.8.198"

#define WIFI_SSID_1 "Dmx's Note20 Ultra"
#define WIFI_PASS_1 "11111129"
#define PC_IP_1     "10.184.253.199"

#define WIFI_SSID_3 "Dialog 4G 208"
#define WIFI_PASS_3 "Hasith2001"
#define PC_IP_3     "192.168.8.198"

#define WIFI_SSID_4 "ENTGRA 5G"
#define WIFI_PASS_4 "Entgra@110"
#define PC_IP_4     "192.168.8.198"

#define SD_FORMAT 1

// --- GLOBAL VARIABLES ---
extern uint8_t esp_is_ready;
extern uint8_t wifi_is_connected;
extern uint8_t tcp_is_connected;
extern uint8_t ntp_sync_status;
extern char current_ip[16];
extern char current_ssid[32];
extern char current_pc_ip[16];

// Function Prototypes
void General_Init(void);
void General_Run(void);
void Scan_I2C_Bus(I2C_HandleTypeDef *hi2c, char *buffer);

#endif /* GENERAL_H */
