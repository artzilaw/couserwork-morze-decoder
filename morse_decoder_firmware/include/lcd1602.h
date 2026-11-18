#ifndef LCD1602_H
#define LCD1602_H

#include "config.h"

void LCD_Init(void);

void LCD_Clear(void);

void LCD_SetCursor(uint8_t col, uint8_t row);

void LCD_SendChar(char c);

void LCD_Print(const char* str);

void LCD_UpdateScreen(DecoderMode_t mode, const char* current_morse, const char* decoded_word);

#endif 
