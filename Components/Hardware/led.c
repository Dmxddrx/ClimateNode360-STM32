#include "led.h"

/* ==================================================================== */
/* COMMON ANODE LOGIC: LOW (Reset) is ON, HIGH (Set) is OFF             */
/* ==================================================================== */
#define RED_ON()    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET)
#define RED_OFF()   HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET)

#define GREEN_ON()  HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET)
#define GREEN_OFF() HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET)

#define BLUE_ON()   HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET)
#define BLUE_OFF()  HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_SET)

/* ==================================================================== */
/* CORE SYSTEM PATTERNS                                                 */
/* ==================================================================== */

void LED_Init(void) {
    // Force all colors OFF on boot
    RED_OFF();
    GREEN_OFF();
    BLUE_OFF();
}

void LED_Boot(void) {
    // Red blink twice
    RED_ON(); HAL_Delay(150); RED_OFF(); HAL_Delay(150);
    RED_ON(); HAL_Delay(150); RED_OFF(); HAL_Delay(150);

    // Green solid for 1 second
    GREEN_ON();
    HAL_Delay(1000);
    GREEN_OFF();
}

void LED_WifiConnecting(void) {
    // Quick Blue pulse when actively trying to reach an Access Point
    BLUE_ON();
    HAL_Delay(100);
    BLUE_OFF();
}

void LED_WifiConnected(void) {
    // Green blink twice
    BLUE_ON();
    HAL_Delay(1000);
    BLUE_OFF();
}

void LED_WifiDisconnected(void) {
    // Red blink twice
    RED_ON(); HAL_Delay(100); RED_OFF(); HAL_Delay(100);
    RED_ON(); HAL_Delay(100); RED_OFF();
}

/* ==================================================================== */
/* CONTINUOUS NON-BLOCKING PATTERNS                                     */
/* ==================================================================== */

void LED_ProcessReconnecting(void) {
    static uint32_t last_tick = 0;
    static uint8_t step = 0;

    uint32_t now = HAL_GetTick();

    // Determine how long to wait based on the current step
    // Step 0 (Red) and Step 2 (Blue) stay on for 150ms.
    // Step 1 and 3 (Off) stay off for 50ms.
    uint32_t delay = (step == 1 || step == 3) ? 50 : 150;

    if (now - last_tick >= delay) {
        last_tick = now;
        step++;
        if (step > 3) step = 0; // Reset loop

        // Execute the color change for this step
        if (step == 0) {
            RED_ON(); BLUE_OFF(); GREEN_OFF();
        } else if (step == 1 || step == 3) {
            RED_OFF(); BLUE_OFF(); GREEN_OFF();
        } else if (step == 2) {
            RED_OFF(); BLUE_ON(); GREEN_OFF();
        }
    }
}

void LED_Off(void) {
    RED_OFF();
    GREEN_OFF();
    BLUE_OFF();
}

/* ==================================================================== */
/* TELEMETRY UPLOAD PATTERNS (Very fast, minimal blocking)              */
/* ==================================================================== */

void LED_UploadSuccess(void) {
    // Single quick green blink (50ms is enough to be visible but not block the CPU)
    GREEN_ON();
    HAL_Delay(50);
    GREEN_OFF();
}

void LED_UploadFail(void) {
    // Single quick red blink
    RED_ON();
    HAL_Delay(50);
    RED_OFF();
}

/* ==================================================================== */
/* HARDWARE ERROR PATTERNS                                              */
/* ==================================================================== */

void LED_HardwareError(void) {
    // 5 rapid red flashes to indicate physical intervention required
    for (int i = 0; i < 5; i++) {
        RED_ON();  HAL_Delay(50);
        RED_OFF(); HAL_Delay(50);
    }
}

/* ==================================================================== */
/* SD CARD FORMAT PATTERNS                                              */
/* ==================================================================== */

void LED_FormatSuccess(void) {
    // 3 distinct green blinks to indicate a successful wipe/format
    for (int i = 0; i < 3; i++) {
        GREEN_ON(); BLUE_OFF();
        HAL_Delay(150);
        GREEN_OFF(); BLUE_ON();
        HAL_Delay(150);
        BLUE_OFF();
        HAL_Delay(150);
    }
}

void LED_FormatFail(void) {
    // 3 long, slow red blinks indicating a critical drive failure
    for (int i = 0; i < 3; i++) {
        BLUE_ON(); RED_ON();
        HAL_Delay(400);
        BLUE_OFF(); RED_OFF();
        HAL_Delay(300);
    }
}
