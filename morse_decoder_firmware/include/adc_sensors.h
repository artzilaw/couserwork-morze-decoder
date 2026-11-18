#ifndef ADC_SENSORS_H
#define ADC_SENSORS_H

#include "config.h"

void Sensors_Init(void);


uint16_t ADC_Read(uint8_t channel);


bool Sensors_CheckSignal(uint32_t current_time_ms);

#endif 
