#include "morse_crypto.h"
#include <string.h>

typedef struct {
    char        letter;
    const char* code;
} MorseEntry_t;

/* Полная таблица кодов Морзе со служебными командами */
static const MorseEntry_t g_morse_table[] = {
    {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
    {'E', "."},    {'F', "..-."}, {'G', "--."},  {'H', "...."},
    {'I', ".."},   {'J', ".---"}, {'K', "-.-"},  {'L', ".-.."},
    {'M', "--"},   {'N', "-."},   {'O', "---"},  {'P', ".--."},
    {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
    {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"},
    {'Y', "-.--"}, {'Z', "--.."},
    {' ', ".-.-"},    
    {'\b', "......"}, 
    {'*', "----"}     
};

char Morse_Decode(const char* code) {
    size_t count = sizeof(g_morse_table) / sizeof(g_morse_table[0]);
    for (size_t i = 0; i < count; i++) {
        if (strcmp(g_morse_table[i].code, code) == 0) {
            return g_morse_table[i].letter;
        }
    }
    return '?';
}

char Crypto_ApplyCaesar(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(((c - 'A' + 3) % 26) + 'A');
    }
    return c;
}

char Crypto_ApplyAtbash(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)('Z' - (c - 'A'));
    }
    return c;
}

const char* Crypto_GetModeName(DecoderMode_t mode) {
    switch (mode) {
        case MODE_MORSE:  return "MORSE";
        case MODE_CAESAR: return "CAESAR";
        case MODE_ATBASH: return "ATBASH";
        default:          return "WAIT";
    }
}
