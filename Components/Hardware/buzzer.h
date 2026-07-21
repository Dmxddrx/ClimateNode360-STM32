#ifndef BUZZER_H
#define BUZZER_H

#include "main.h"
#include <stdint.h>

// Core Tone Generator
// frequency: in Hz (e.g., 440 for Middle A). Pass 0 for silence.
// duration_ms: how long to play the tone in milliseconds.
void Buzzer_PlayTone(uint16_t frequency, uint32_t duration_ms);

// Custom Sound Profiles
void Buzzer_BootSound(void);
void Buzzer_WifiConnected(void);
void Buzzer_WifiDisconnected(void);
void Buzzer_LongPressAck(void);
void Buzzer_FormatSuccess(void);
void Buzzer_FormatFail(void);
void Buzzer_SD_Inserted(void);
void Buzzer_SD_Removed(void);

#endif /* BUZZER_H */
