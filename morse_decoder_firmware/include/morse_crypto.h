#ifndef MORSE_CRYPTO_H
#define MORSE_CRYPTO_H

#include "config.h"

char Morse_Decode(const char* code);

char Crypto_ApplyCaesar(char c);

char Crypto_ApplyAtbash(char c);

const char* Crypto_GetModeName(DecoderMode_t mode);

#endif 
