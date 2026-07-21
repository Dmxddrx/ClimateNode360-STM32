#include "wifi.h"
#include "main.h"
#include "oled.h"
#include "led.h"

extern IWDG_HandleTypeDef hiwdg;

char buffer[512];

void WIFI_SendCommand(const char* command) {
    /* 1. Prevent collisions: Wait for any active background DMA transfers to finish */
    while (huart2.gState != HAL_UART_STATE_READY) {
        HAL_IWDG_Refresh(&hiwdg); // Keep petting the dog while we wait!
    }

    /* 2. Give the ESP8266 a tiny 10ms breathing window to finish processing */
    HAL_Delay(10);

    /* 3. Safe to transmit */
    HAL_UART_Transmit(&huart2, (uint8_t*)command, strlen(command), 100);
}

int8_t WIFI_WaitForResponse(const char* expected_response, uint32_t timeout) {
    uint32_t startTime = HAL_GetTick();
    memset(buffer, 0, sizeof(buffer));
    uint16_t index = 0;

    // --- CRITICAL FIX: CLEAR OVERRUN ERRORS ---
    // Read SR and DR to clear the hardware ORE flag caused by abandoned "SEND OK" messages
    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    // Force the HAL state machine to forget the error so it can receive again
    huart2.ErrorCode = HAL_UART_ERROR_NONE;
    huart2.RxState = HAL_UART_STATE_READY;
    // ------------------------------------------

    while (HAL_GetTick() - startTime < timeout) {

    	HAL_IWDG_Refresh(&hiwdg);

        uint8_t data;
        if (HAL_UART_Receive(&huart2, &data, 1, 1) == HAL_OK) {
            // SAFETY FIX: Prevent buffer overflow if expected response never arrives
            if (index < 511) {
                buffer[index++] = data;
            }
            if (strstr(buffer, expected_response)) {
                return 1; // Success
            }
        }
    }
    return 0; // Timeout
}

int8_t WIFI_Init(void) {
    // --- ADVANCED HARDWARE BOOT & RESET SEQUENCE ---

    // 1. Force the ESP into Normal Boot Mode by pulling GPIO0 HIGH
	HAL_GPIO_WritePin(ESP_I00_GPIO_Port, ESP_I00_Pin, GPIO_PIN_SET);

    // 2. Pull RST LOW to hard-reset the ESP
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);

    // 3. Pull RST HIGH to let it boot (while GPIO0 is still held HIGH)
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_SET);

    // 4. Give the ESP-01S 1 second to fully boot up
    HAL_Delay(800);

    // 5. Release GPIO0 (Set it back to an input)
    // This prevents the STM32 from interfering if the ESP tries to use the pin later.
    GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = ESP_I00_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(ESP_I00_GPIO_Port, &GPIO_InitStruct);

    // 6. Flush the UART data register to clear out the boot garbage
    // (The ESP spits out a bunch of random text at 74880 baud when it first turns on)
    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    // ------------------------------------------------

    // --- NEW FIX: DISABLE ECHO ---
	// This stops the ESP from repeating our own commands back to us
	WIFI_SendCommand("ATE0\r\n");
	WIFI_WaitForResponse("OK", 500);
	// -----------------------------

    // Now proceed with normal AT Commands
    // Inside wifi.c - WIFI_Init
    WIFI_SendCommand("AT\r\n");
    if (!WIFI_WaitForResponse("OK", 500)) {
        // Optional: Print the first 10 characters of whatever the ESP actually sent
        OLED_Print(0, 50, buffer);
        OLED_Update();
        return 0;
    }

    WIFI_SendCommand("AT+CWMODE=1\r\n");
    if (!WIFI_WaitForResponse("OK", 500)) return 0;

    return 1; // Init successful
}

int8_t WIFI_Connect(const char* ssid, const char* password) {
    char conn_cmd[128];
    sprintf(conn_cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);

    WIFI_SendCommand(conn_cmd);

    // --- CUSTOM NON-BLOCKING WAIT LOOP ---
    uint32_t startTime = HAL_GetTick();
    memset(buffer, 0, sizeof(buffer));
    uint16_t index = 0;

    // Clear the hardware UART errors so we can listen cleanly
    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    huart2.ErrorCode = HAL_UART_ERROR_NONE;
    huart2.RxState = HAL_UART_STATE_READY;

    // Wait up to 15 seconds for the router...
    while (HAL_GetTick() - startTime < 15000) {
        HAL_IWDG_Refresh(&hiwdg);

        // --- CONTINUOUS BLINK MAGIC ---
        // This toggles the Red/Blue pattern in the background!
        LED_ProcessReconnecting();

        uint8_t data;
        // Listen for data from the ESP8266 (1ms timeout per byte)
        if (HAL_UART_Receive(&huart2, &data, 1, 1) == HAL_OK) {
            if (index < 511) {
                buffer[index++] = data;
            }
            // Did we get an IP address?
            if (strstr(buffer, "WIFI GOT IP")) {
                LED_Off(); // Clean up the LED before exiting
                return 1;  // Success!
            }
        }
    }

    LED_Off(); // Clean up the LED if we timed out
    return 0;  // Failure!
}

// Cleanly disconnects from any stuck connection attempts
void WIFI_Disconnect(void) {
    WIFI_SendCommand("AT+CWQAP\r\n");
    WIFI_WaitForResponse("OK", 1000);
}

int8_t WIFI_GetIP(char* ip_out) {
    WIFI_SendCommand("AT+CIFSR\r\n");

    // The response looks like: +CIFSR:STAIP,"192.168.1.15"
    if (WIFI_WaitForResponse("OK", 1000)) {
        char* start = strstr(buffer, "STAIP,\"");
        if (start) {
            start += 7; // Move past STAIP,"
            char* end = strchr(start, '\"');
            if (end) {
                size_t len = end - start;
                strncpy(ip_out, start, len);
                ip_out[len] = '\0'; // Null terminate
                return 1;
            }
        }
    }
    return 0;
}

// WIFI_StartUDP tells the ESP to target your PC.
int8_t WIFI_StartUDP(const char* target_ip, uint16_t port) {
    char cmd[128];
    // Command format: AT+CIPSTART="UDP","192.168.1.50",8080
    sprintf(cmd, "AT+CIPSTART=\"UDP\",\"%s\",%d\r\n", target_ip, port);

    WIFI_SendCommand(cmd);

    // Wait up to 2 seconds for the ESP to confirm the connection
    return WIFI_WaitForResponse("OK", 2000);
}


// WIFI_SendUDPData handles the tricky 2-step process.
// Uses standard blocking for the short setup, and DMA for the massive payload.
int8_t WIFI_SendUDPData(const char* data) {
    char cmd[32];
    uint16_t len = strlen(data);

    /* Check if UART is still busy from the previous DMA transfer (Safety Catch) */
    if (huart2.gState != HAL_UART_STATE_READY) {
        return 0; /* Skip this cycle to prevent memory corruption */
    }

    // 1. Tell ESP how many bytes we want to send (Blocking, it's very short)
    sprintf(cmd, "AT+CIPSEND=%d\r\n", len);
    WIFI_SendCommand(cmd);

    // 2. Wait for the '>' prompt from the ESP indicating it is ready (Takes ~1ms)
    if (WIFI_WaitForResponse(">", 5)) {

        // 3. THE DMA UPGRADE: Send the massive sensor data string in the background!
        HAL_UART_Transmit_DMA(&huart2, (uint8_t*)data, len);

        // 4. Do NOT wait for "SEND OK". Instantly return so the CPU can go back
        // to balancing Moppy's motors while the DMA controller handles the Wi-Fi!
        return 1;
    }
    return 0; // Failed
}

int8_t WIFI_IsConnected(void) {
    /* Ask the ESP8266 what router it is currently connected to */
    WIFI_SendCommand("AT+CWJAP?\r\n");

    /* Look for a quote mark (") which signifies it is returning an SSID string.
       This proves it is actively connected, and guarantees no false positives. */
    if (WIFI_WaitForResponse("\"", 1000)) {
        /* Consume the trailing "OK" so the buffer is clean */
        WIFI_WaitForResponse("OK", 500);
        return 1; // Verified online
    }

    /* If it timed out or responded with "No AP", it is offline */
    return 0;
}
