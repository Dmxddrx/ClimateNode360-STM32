#ifndef LED_H
#define LED_H

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

// Telemetry Prototypes
void LED_UploadSuccess(void);
void LED_UploadFail(void);

// Hardware Error Prototype
void LED_HardwareError(void);

void LED_FormatSuccess(void);
void LED_FormatFail(void);

#endif /* LED_H */
