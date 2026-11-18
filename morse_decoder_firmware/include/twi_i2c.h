#ifndef TWI_I2C_H
#define TWI_I2C_H

#include "config.h"

void TWI_Init(void);

void TWI_StartWrite(uint8_t slave_addr);

void TWI_WriteByte(uint8_t data);

void TWI_Stop(void);

#endif 
