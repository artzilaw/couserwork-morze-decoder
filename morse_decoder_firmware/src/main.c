#include "config.h"
#include "timer.h"
#include "uart.h"
#include "adc_sensors.h"
#include "lcd1602.h"
#include "encoder.h"
#include "morse_crypto.h"
#include <string.h>

static void Str_AppendChar(char* buf, size_t max_len, char c) {
    size_t len = strlen(buf);
    if ((len + 1U) < max_len) {
        buf[len]      = c;
        buf[len + 1U] = '\0';
    }
}

static void Str_RemoveLast(char* buf) {
    size_t len = strlen(buf);
    if (len > 0U) {
        buf[len - 1U] = '\0';
    }
}

int main(void) {
    Timer0_Init();
    USART0_Init();
    Sensors_Init();
    Encoder_Init();
    LCD_Init();

    sei();

    LCD_SetCursor(0, 0);
    LCD_Print("Crypto Decoder");
    LCD_SetCursor(0, 1);
    LCD_Print("Prefix / Encoder");
    Timer0_DelayMs(2000);
    LCD_Clear();

    DecoderMode_t current_mode     = MODE_WAIT;
    DecoderMode_t last_active_mode = MODE_MORSE;

    char current_morse[MAX_MORSE_LEN] = "";
    char decoded_word[MAX_WORD_LEN]   = "";

    bool     is_pressed       = false;
    uint32_t press_start_time = 0;
    uint32_t release_time     = 0;

    LCD_UpdateScreen(current_mode, current_morse, decoded_word);

    while (1) {
        uint32_t current_time = Timer0_Millis();

        if (Encoder_Process(current_time,
                            &current_mode,
                            &last_active_mode,
                            current_morse,
                            decoded_word)) {
            LCD_UpdateScreen(current_mode, current_morse, decoded_word);
        }

        if (USART0_Available()) {
            char incoming_char = USART0_ReadChar();
            if (incoming_char != '\n' && incoming_char != '\r' && incoming_char != '\0') {
                if (incoming_char == '\b') {
                    Str_RemoveLast(decoded_word);
                } else {
                    Str_AppendChar(decoded_word, MAX_WORD_LEN, incoming_char);
                }
                LCD_UpdateScreen(current_mode, current_morse, decoded_word);
            }
        }

        bool signal_active = Sensors_CheckSignal(current_time);

        if (signal_active && !is_pressed) {
            is_pressed       = true;
            press_start_time = current_time;
            Timer2_Tone(1000); 
        }

        if (!signal_active && is_pressed) {
            is_pressed = false;
            uint32_t press_duration = current_time - press_start_time;
            Timer2_NoTone();
            release_time = current_time;

            if (press_duration < MORSE_DOT_MAX_MS) {
                Str_AppendChar(current_morse, MAX_MORSE_LEN, '.');
            } else {
                Str_AppendChar(current_morse, MAX_MORSE_LEN, '-');
            }
            LCD_UpdateScreen(current_mode, current_morse, decoded_word);
        }

        if (!is_pressed && (current_morse[0] != '\0') &&
            ((current_time - release_time) > MORSE_CHAR_GAP_MS)) {

            if (current_mode == MODE_WAIT) {
                if (strcmp(current_morse, "...") == 0) {
                    current_mode     = MODE_MORSE;
                    last_active_mode = MODE_MORSE;
                } else if (strcmp(current_morse, "---") == 0) {
                    current_mode     = MODE_CAESAR;
                    last_active_mode = MODE_CAESAR;
                } else if (strcmp(current_morse, ".-.") == 0) {
                    current_mode     = MODE_ATBASH;
                    last_active_mode = MODE_ATBASH;
                }
                current_morse[0] = '\0';
                LCD_UpdateScreen(current_mode, current_morse, decoded_word);
                continue;
            }

            char decoded_letter = Morse_Decode(current_morse);

            if (decoded_letter == '*') {
                current_mode = MODE_WAIT;
            } else if (decoded_letter == ' ') {
                Str_AppendChar(decoded_word, MAX_WORD_LEN, ' ');
                USART0_SendJSON("COMMAND", "SPACE");
            } else if (decoded_letter == '\b') {
                Str_RemoveLast(decoded_word);
                USART0_SendJSON("COMMAND", "BACKSPACE");
            } else if (decoded_letter != '?') {
                char final_char = decoded_letter;

                if (current_mode == MODE_CAESAR) {
                    final_char = Crypto_ApplyCaesar(final_char);
                } else if (current_mode == MODE_ATBASH) {
                    final_char = Crypto_ApplyAtbash(final_char);
                }

                Str_AppendChar(decoded_word, MAX_WORD_LEN, final_char);

                char char_str[2] = { final_char, '\0' };
                USART0_SendJSON(Crypto_GetModeName(current_mode), char_str);
            }

            current_morse[0] = '\0';
            LCD_UpdateScreen(current_mode, current_morse, decoded_word);
        }
    }

    return 0;
}
