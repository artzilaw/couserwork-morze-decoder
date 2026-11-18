#ifndef ENCODER_H
#define ENCODER_H

#include "config.h"

void Encoder_Init(void);

bool Encoder_Process(uint32_t now_ms,
                     DecoderMode_t* current_mode,
                     DecoderMode_t* last_active_mode,
                     char* current_morse,
                     char* decoded_word);

#endif 
