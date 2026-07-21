#include "btns.h"

// Renamed from DEBOUNCE to COOLDOWN.
// 300ms prevents rapid multi-paging if a finger lingers on the sensor.
#define TOUCH_COOLDOWN_MS   300

// Idle state for a touch sensor is 0 (LOW)
static uint8_t   last_btn_oledpage  = 0;
static uint32_t  debounce_tick      = 0;
static BTN_State oledpage_event     = BTN_IDLE;

uint8_t format_requested = 0;
static uint32_t press_start_tick = 0;
static uint8_t is_pressing = 0;
/* ================================================================
   BTNS_Init
   GPIO already configured in CubeMX — nothing to init here.
   ================================================================ */
void BTNS_Init(void)
{
    /* nothing needed — GPIO init done by MX_GPIO_Init in main.c */
}

/* ================================================================
   BTNS_Update — call every loop
   Detects rising edge (touch applied) with a cooldown timer.
   ================================================================ */
void BTNS_Update(void) {
    uint8_t btn = HAL_GPIO_ReadPin(BTN_OLED_PAGE_GPIO_Port, BTN_OLED_PAGE_Pin);

    /* RISING EDGE = Touch sensor detected a finger */
    if(last_btn_oledpage == 0 && btn == 1) {
        if(HAL_GetTick() - debounce_tick >= TOUCH_COOLDOWN_MS) {
            oledpage_event = BTN_PRESSED;
            debounce_tick  = HAL_GetTick();

            // Start timing the press
            press_start_tick = HAL_GetTick();
            is_pressing = 1;
        }
    }
    /* HELD DOWN = Finger resting on sensor */
    else if (is_pressing && btn == 1) {
        // If held for 3 continuous seconds
        if ((HAL_GetTick() - press_start_tick) > 3000) {
            format_requested = 1;
            is_pressing = 0; // Lockout to prevent rapid-fire formatting
        }
    }
    /* RELEASED = Finger removed */
    else if (btn == 0) {
        is_pressing = 0;
    }

    last_btn_oledpage = btn;
}

/* ================================================================
   BTNS_Get_OLEDPage
   Returns BTN_PRESSED once then clears — like reading a flag.
   ================================================================ */
BTN_State BTNS_Get_OLEDPage(void)
{
    BTN_State state = oledpage_event;
    oledpage_event  = BTN_IDLE;   /* clear after read */
    return state;
}
