#ifndef CONFIG_H
#define CONFIG_H

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>


#define BTN_MORSE_DDR       DDRD
#define BTN_MORSE_PORT      PORTD
#define BTN_MORSE_PIN_REG   PIND
#define BTN_MORSE_BIT       PD2

#define ENC_DDR             DDRD
#define ENC_PORT            PORTD
#define ENC_PIN_REG         PIND
#define ENC_CLK_BIT         PD3
#define ENC_DT_BIT          PD4
#define ENC_SW_BIT          PD5

#define BUZZER_DDR          DDRB
#define BUZZER_PORT         PORTB
#define BUZZER_BIT          PB0

#define ADC_CH_PHOTO        0
#define ADC_CH_SOUND        1

#define LCD_I2C_ADDR        0x27

#define UART_BAUD_RATE      9600UL


#define SOUND_THRESHOLD     120U    
#define SOUND_HOLD_MS       100UL   
#define LIGHT_THRESHOLD     600U    

#define NOISE_FILTER_MS     25UL    
#define MORSE_DOT_MAX_MS    300UL   
#define MORSE_CHAR_GAP_MS   700UL   

#define ENC_DEBOUNCE_MS     40UL    
#define ENC_LONG_PRESS_MS   700UL   

#define MAX_MORSE_LEN       12
#define MAX_WORD_LEN        64

typedef enum {
    MODE_WAIT = 0,
    MODE_MORSE,
    MODE_CAESAR,
    MODE_ATBASH
} DecoderMode_t;

#endif 
