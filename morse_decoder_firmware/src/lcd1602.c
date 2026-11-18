#include "lcd1602.h"
#include "twi_i2c.h"
#include <util/delay.h>
#include <string.h>


#define PCF_RS  (1 << 0)
#define PCF_RW  (1 << 1)
#define PCF_EN  (1 << 2)
#define PCF_BL  (1 << 3)

static void LCD_WriteI2C(uint8_t byte_val) {
    TWI_StartWrite(LCD_I2C_ADDR);
    TWI_WriteByte(byte_val | PCF_BL);
    TWI_Stop();
}

static void LCD_PulseEnable(uint8_t data) {
    LCD_WriteI2C(data | PCF_EN);
    _delay_us(2);
    LCD_WriteI2C(data & (uint8_t)(~PCF_EN));
    _delay_us(50);
}

static void LCD_Write4Bits(uint8_t nibble_shifted, uint8_t mode_rs) {
    uint8_t packet = (uint8_t)((nibble_shifted & 0xF0) | (mode_rs ? PCF_RS : 0x00) | PCF_BL);
    LCD_WriteI2C(packet);
    LCD_PulseEnable(packet);
}

static void LCD_SendByte(uint8_t val, uint8_t mode_rs) {
    LCD_Write4Bits(val & 0xF0, mode_rs);
    LCD_Write4Bits((uint8_t)((val << 4) & 0xF0), mode_rs);
}

static void LCD_SendCommand(uint8_t cmd) {
    LCD_SendByte(cmd, 0);
}

void LCD_SendChar(char c) {
    LCD_SendByte((uint8_t)c, 1);
}

void LCD_Init(void) {
    TWI_Init();
    _delay_ms(50); 
    LCD_Write4Bits(0x30, 0);
    _delay_ms(5);
    LCD_Write4Bits(0x30, 0);
    _delay_ms(5);
    LCD_Write4Bits(0x30, 0);
    _delay_us(150);
    LCD_Write4Bits(0x20, 0); 
    _delay_us(150);

    LCD_SendCommand(0x28); 
    LCD_SendCommand(0x0C);
    LCD_SendCommand(0x06); 
    LCD_Clear();
}

void LCD_Clear(void) {
    LCD_SendCommand(0x01);
    _delay_ms(2);
}

void LCD_SetCursor(uint8_t col, uint8_t row) {
    uint8_t row_offset = (row == 0) ? 0x00 : 0x40;
    LCD_SendCommand((uint8_t)(0x80 | (col + row_offset)));
}

void LCD_Print(const char* str) {
    while (*str != '\0') {
        LCD_SendChar(*str++);
    }
}

void LCD_UpdateScreen(DecoderMode_t mode, const char* current_morse, const char* decoded_word) {
    LCD_SetCursor(0, 0);
    switch (mode) {
        case MODE_WAIT:
            LCD_Print("MODE: WAIT      ");
            break;
        case MODE_MORSE:
            LCD_Print("MORSE:          ");
            break;
        case MODE_CAESAR:
            LCD_Print("CAESAR:         ");
            break;
        case MODE_ATBASH:
            LCD_Print("ATBASH:         ");
            break;
        default:
            LCD_Print("MODE: WAIT      ");
            break;
    }

    LCD_SetCursor(8, 0);
    uint8_t m_len = 0;
    while (current_morse[m_len] != '\0' && m_len < 8) {
        LCD_SendChar(current_morse[m_len]);
        m_len++;
    }
    while (m_len < 8) {
        LCD_SendChar(' ');
        m_len++;
    }

    LCD_SetCursor(0, 1);
    size_t w_len = strlen(decoded_word);
    const char* view_ptr = decoded_word;
    if (w_len > 16U) {
        view_ptr = decoded_word + (w_len - 16U);
        w_len = 16U;
    }

    for (size_t i = 0; i < 16U; i++) {
        if (i < w_len) {
            LCD_SendChar(view_ptr[i]);
        } else {
            LCD_SendChar(' ');
        }
    }
}
