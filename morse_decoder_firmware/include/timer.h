#ifndef TIMER_H
#define TIMER_H

#include "config.h"

void Timer0_Init(void);

uint32_t Timer0_Millis(void);

void Timer0_DelayMs(uint16_t ms);

void Timer2_Tone(uint16_t freq_hz);

void Timer2_NoTone(void);

void Buzzer_Beep(uint16_t duration_ms, uint16_t freq_hz);

#endif 
