#ifndef UART_H
#define UART_H

#include "config.h"

void USART0_Init(void);

void USART0_SendChar(char c);

void USART0_SendString(const char* str);

void USART0_SendLine(const char* str);

void USART0_SendJSON(const char* mode, const char* ch);

bool USART0_Available(void);

char USART0_ReadChar(void);

#endif 
