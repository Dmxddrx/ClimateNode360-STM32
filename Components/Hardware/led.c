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
/* COLOR MIXING MACROS                                                  */
/* ==================================================================== */
#define COLOR_OFF()     { RED_OFF(); GREEN_OFF(); BLUE_OFF(); }
#define COLOR_RED()     { RED_ON();  GREEN_OFF(); BLUE_OFF(); }
#define COLOR_GREEN()   { RED_OFF(); GREEN_ON();  BLUE_OFF(); }
#define COLOR_BLUE()    { RED_OFF(); GREEN_OFF(); BLUE_ON();  }
#define COLOR_YELLOW()  { RED_ON();  GREEN_ON();  BLUE_OFF(); }
#define COLOR_CYAN()    { RED_OFF(); GREEN_ON();  BLUE_ON();  }
#define COLOR_MAGENTA() { RED_ON();  GREEN_OFF(); BLUE_ON();  }
#define COLOR_WHITE()   { RED_ON();  GREEN_ON();  BLUE_ON();  }

/* ==================================================================== */
/* CORE SYSTEM PATTERNS                                                 */
/* ==================================================================== */

void LED_Init(void) {
    // Force all colors OFF on boot
	COLOR_OFF();
}

void LED_Boot(void) {
	// White blink twice to indicate core system boot
	COLOR_WHITE(); HAL_Delay(150); COLOR_OFF(); HAL_Delay(150);
	COLOR_WHITE(); HAL_Delay(150); COLOR_OFF(); HAL_Delay(150);

	// Solid White for 1 second
	COLOR_GREEN()
	HAL_Delay(1000);
	COLOR_OFF();
}

/* ==================================================================== */
/* WIFI PATTERNS (Blue)                                                 */
/* ==================================================================== */

void LED_WifiConnecting(void) {
    // Quick Blue pulse when actively trying to reach an Access Point
	COLOR_BLUE();
	HAL_Delay(100);
	COLOR_OFF();
}

void LED_WifiConnected(void) {
	COLOR_BLUE();
	HAL_Delay(1000);
	COLOR_OFF();
}

void LED_WifiDisconnected(void) {
	// Blue to Red blink indicating radio failure
	COLOR_BLUE(); HAL_Delay(100); COLOR_OFF(); HAL_Delay(100);
	COLOR_RED();  HAL_Delay(100); COLOR_OFF();
}

/* ==================================================================== */
/* CONTINUOUS NON-BLOCKING PATTERNS                                     */
/* ==================================================================== */

void LED_ProcessReconnecting(void) {
    static uint32_t last_tick = 0; //[cite: 2]
    static uint8_t step = 0;       //[cite: 2]

    uint32_t now = HAL_GetTick();  //[cite: 2]

    uint32_t delay = (step == 1 || step == 3) ? 50 : 150; //[cite: 2]

    if (now - last_tick >= delay) { //[cite: 2]
        last_tick = now;            //[cite: 2]
        step++;                     //[cite: 2]
        if (step > 3) step = 0;     //[cite: 2]

        // Yellow (Warning) to Blue (Radio) pulse
        if (step == 0) {
            COLOR_YELLOW();
        } else if (step == 1 || step == 3) {
            COLOR_OFF();
        } else if (step == 2) {
            COLOR_BLUE();
        }
    }
}

void LED_Off(void) {
	COLOR_OFF(); //[cite: 2]
}

/* ==================================================================== */
/* TELEMETRY UPLOAD PATTERNS (Green/Red)                                */
/* ==================================================================== */

void LED_UploadSuccess(void) {
    // Fast Green blink[cite: 2]
    COLOR_GREEN();
    HAL_Delay(50);
    COLOR_OFF();
}

void LED_UploadFail(void) {
    // Fast Cyan to Red blink indicating data layer failure
    COLOR_CYAN(); HAL_Delay(50);
    COLOR_RED();  HAL_Delay(50);
    COLOR_OFF();
}

/* ==================================================================== */
/* HARDWARE ERROR PATTERNS (Red/Magenta)                                */
/* ==================================================================== */

void LED_HardwareError(void) {
    // 5 rapid red flashes[cite: 2]
    for (int i = 0; i < 5; i++) {
        COLOR_RED(); HAL_Delay(50);
        COLOR_OFF(); HAL_Delay(50);
    }
}

/* ==================================================================== */
/* SD CARD FORMAT PATTERNS (Magenta)                                    */
/* ==================================================================== */

void LED_FormatSuccess(void) {
    // 3 Magenta to Green transitions
    for (int i = 0; i < 3; i++) {
        COLOR_MAGENTA(); HAL_Delay(150);
        COLOR_GREEN();   HAL_Delay(150);
        COLOR_OFF();     HAL_Delay(150);
    }
}

void LED_FormatFail(void) {
    // 3 long Magenta to Red blinks
    for (int i = 0; i < 3; i++) {
        COLOR_MAGENTA(); HAL_Delay(400);
        COLOR_RED();     HAL_Delay(400);
        COLOR_OFF();     HAL_Delay(300);
    }
}

/* ==================================================================== */
/* TCP & NTP PATTERNS (Cyan)                                            */
/* ==================================================================== */

void LED_TcpRetry(void) {
    // Quick double Cyan flash
    COLOR_CYAN(); HAL_Delay(50); COLOR_OFF(); HAL_Delay(50);
    COLOR_CYAN(); HAL_Delay(50); COLOR_OFF();
}

void LED_TcpConnected(void) {
    // One solid Cyan flash
    COLOR_CYAN(); HAL_Delay(200); COLOR_OFF();
}

void LED_TcpFailed(void) {
    // Cyan to Red flash
    COLOR_CYAN(); HAL_Delay(100);
    COLOR_RED();  HAL_Delay(200);
    COLOR_OFF();
}

void LED_NtpFailed(void) {
    // Cyan to Yellow pulse to show a minor timing warning
    COLOR_CYAN();   HAL_Delay(100);
    COLOR_YELLOW(); HAL_Delay(200);
    COLOR_OFF();
}
