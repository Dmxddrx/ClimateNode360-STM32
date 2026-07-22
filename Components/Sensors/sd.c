#include "sd.h"

extern Disk_drvTypeDef disk;

// Global FATFS variables
static FATFS fs;
static FIL log_file;
static UINT bytes_written;

static uint8_t sd_is_ready = 0;

// --- NEW: RAM Tracking Variables ---
static uint32_t total_data_rows = 0;
static char last_logged_data[32] = "No Data Yet";

uint8_t SD_Init(void) {
	// --- CRITICAL FIX: Bypass the CubeMX initialization lock ---
	disk.is_initialized[0] = 0;

	// Unmount any ghost drives lingering in the RAM
	f_mount(NULL, "", 0);
	// -----------------------------------------------------------

    // 1. Mount the SD Card immediately (1 = force mount)
    if (f_mount(&fs, "", 1) == FR_OK) {

    	// Pre-scan file to count existing rows on boot
		FIL scan_file;
		total_data_rows = 0;

		// Open in READ mode to count newlines quickly
		if (f_open(&scan_file, "data.csv", FA_READ | FA_OPEN_EXISTING) == FR_OK) {
			char buf[128];
			UINT br;
			while (f_read(&scan_file, buf, sizeof(buf), &br) == FR_OK && br > 0) {
				HAL_IWDG_Refresh(&hiwdg); // Feed dog during heavy scan
				for (UINT i = 0; i < br; i++) {
					if (buf[i] == '\n') {
						total_data_rows++;
					}
				}
			}
			f_close(&scan_file);

			// Subtract 1 to exclude the header row from the data count
			if (total_data_rows > 0) total_data_rows--;
		}
		// ---------------------------------------------------------

        // 2. Open or create the file
        if (f_open(&log_file, "data.csv", FA_OPEN_ALWAYS | FA_WRITE) == FR_OK) {

            // 3. MOVE THE POINTER TO THE END OF THE FILE
            f_lseek(&log_file, f_size(&log_file));

            // 4. Update the CSV Header row to include Humidity
            if (f_size(&log_file) == 0) {
				const char* header = "Date,Time,Device,Temperature(C),Humidity(%),Status\n"; // <--- UPDATED
				f_write(&log_file, header, strlen(header), &bytes_written);
			}

            // 5. Safely close the file
            f_close(&log_file);

            sd_is_ready = 1; // SUCCESS: Unlock the SD logging functions
            return 1;
        }
    }

    sd_is_ready = 0; // FAILURE: Lock the SD logging functions
    return 0;
}

uint8_t SD_LogData(float temperature, float humidity, int16_t dust, const char* status) {
    char buffer[128];
    uint8_t success = 0;

    // BUG FIX: Instantly exit if the SD card failed to initialize during boot.
    // This allows the Wi-Fi and OLED to continue running smoothly.
    if (!sd_is_ready) {
        return 0;
    }

    // 1. Format the data into a standard CSV string including humidity and dust
    RTC_TimeTypeDef now;
	char time_str[32] = "1970-01-01,00:00:00"; // Fallback if RTC fails

	if (RTC_GetTime(&now)) {
		// Format as "YYYY-MM-DD,HH:MM:SS"
		snprintf(time_str, sizeof(time_str), "20%02d-%02d-%02d,%02d:%02d:%02d",
				 now.Year, now.Month, now.Date, now.Hour, now.Minutes, now.Seconds);
	}
	snprintf(buffer, sizeof(buffer), "%s,Node360,%.1f,%.1f,%.1f,%s\n",
	             time_str, temperature, humidity, (float)dust / 10.0f, status);

    // 2. Open the file
    if (f_open(&log_file, "data.csv", FA_OPEN_ALWAYS | FA_WRITE) == FR_OK) {

        // 3. CRITICAL: Jump to the end of the file before writing!
        f_lseek(&log_file, f_size(&log_file));

        // 4. Write the string to the SD card
		if (f_write(&log_file, buffer, strlen(buffer), &bytes_written) == FR_OK) {
			if (bytes_written == strlen(buffer)) {
				success = 1;

				// --- NEW: Update live stats ---
				total_data_rows++;
				// Format a compact 21-character string to fit perfectly on the OLED
				snprintf(last_logged_data, sizeof(last_logged_data), "%.1fC %.0f%% %dug",
						 temperature, humidity, (int)(dust / 10));
				// ------------------------------
			} else {
                sd_is_ready = 0; // Offline: Bytes didn't write
            }
		} else {
            sd_is_ready = 0; // Offline: Disk Error during write
        }

        // 5. Close the file immediately to prevent corruption
        f_close(&log_file);
    } else {
        // Safety Catch: If the card is suddenly removed while running,
        // lock the SD flag so it doesn't hang on the next loop.
        sd_is_ready = 0;
    }

    return success;
}

#if SD_FORMAT
uint8_t SD_Format(void) {
    FRESULT res;
    char err_buf[32];

    // 1. Attempt to Mount
    res = f_mount(&fs, "", 1);
    if (res != FR_OK) {
        // If mount fails, it will print exactly WHY it failed (e.g., "Mount Err: 3")
        snprintf(err_buf, sizeof(err_buf), "Mount Err: %d", res);
        OLED_Print(0, 36, err_buf);
        return 0;
    }

    // 2. Attempt to Format
    res = f_mkfs("", 0, 0);
    if (res != FR_OK) {
        // If format fails, print exactly why (e.g., "Format Err: 14")
        snprintf(err_buf, sizeof(err_buf), "Format Err: %d", res);
        OLED_Print(0, 36, err_buf);
        f_mount(NULL, "", 0); // Unmount
        return 0;
    }

    // 3. Success
    f_mount(NULL, "", 0);
    return 1;
}
#endif

uint8_t SD_IsReady(void) {
    return sd_is_ready;
}

void SD_GetLogStats(uint32_t *rows, char *last_line) {
    *rows = total_data_rows;
    strncpy(last_line, last_logged_data, 32);
}

void SD_ClearLog(void) {
    if (!sd_is_ready) return;

    // f_unlink deletes the file completely from the FAT32 system
    if (f_unlink("data.csv") == FR_OK) {
        total_data_rows = 0;
        strncpy(last_logged_data, "No Data Yet", 32);
    }
}
