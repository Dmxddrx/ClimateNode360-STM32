#ifndef LED_H
#define LED_H

/* ===================================================================================
 * LED PATTERN VISUAL CHEAT SHEET
 * ===================================================================================
 *
 * --- CORE & SYSTEM (White / Yellow) ---
 * LED_Init                : All colors forced OFF.
 * LED_Boot                : White blink twice -> Solid White (1 sec).
 *
 * --- WIFI LINK LAYER (Blue) ---
 * LED_WifiConnecting      : Quick Blue pulse (100ms).
 * LED_WifiConnected       : Solid Blue (1 sec).
 * LED_WifiDisconnected    : Blue blink -> Red blink.
 *
 * --- CONTINUOUS PROCESSES ---
 * LED_ProcessReconnecting : Non-blocking Yellow (Warning) -> Blue (Radio) pulse.
 * LED_Off                 : All colors forced OFF.
 *
 * --- TELEMETRY UPLOAD (Green / Cyan / Red) ---
 * LED_UploadSuccess       : Fast Green blink (50ms).
 * LED_UploadFail          : Fast Cyan -> Red blink.
 *
 * --- TCP & NTP PROTOCOLS (Cyan / Yellow / Red) ---
 * LED_TcpRetry            : Quick double Cyan flash.
 * LED_TcpConnected        : Solid Cyan flash (200ms).
 * LED_TcpFailed           : Cyan -> Red flash.
 * LED_NtpFailed           : Cyan -> Yellow pulse (Timing warning).
 *
 * --- HARDWARE & STORAGE (Red / Magenta / Green) ---
 * LED_HardwareError       : 5 rapid Red flashes (Critical physical error).
 * LED_FormatSuccess       : 3 Magenta -> Green transitions (Format complete).
 * LED_FormatFail          : 3 long Magenta -> Red blinks (Critical drive failure).
 * =================================================================================== */

#include "main.h"

// Core initialization
void LED_Init(void);

// Core Pattern Prototypes
void LED_Boot(void);
void LED_WifiConnecting(void);
void LED_WifiConnected(void);
void LED_WifiDisconnected(void);
void LED_ProcessReconnecting(void);
void LED_Off(void);

// --- NEW: TCP & NTP Prototypes ---
void LED_TcpRetry(void);
void LED_TcpConnected(void);
void LED_TcpFailed(void);
void LED_NtpFailed(void);

// Telemetry Prototypes
void LED_UploadSuccess(void);
void LED_UploadFail(void);

// Hardware Error Prototype
void LED_HardwareError(void);
void LED_FormatSuccess(void);
void LED_FormatFail(void);

#endif /* LED_H */
