#include "encoder.h"
#include "timer.h"
#include "uart.h"

static const int8_t g_enc_states[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

static volatile uint8_t g_enc_ab       = 0x03;
static volatile int8_t  g_enc_substep  = 0;
static volatile int8_t  g_enc_delta    = 0;

static bool     g_sw_pressed           = false;
static bool     g_sw_long_handled      = false;
static uint32_t g_sw_press_time        = 0;
static uint32_t g_last_rotate_time     = 0;

ISR(PCINT2_vect) {
    uint8_t pind_val = ENC_PIN_REG;
    uint8_t clk = (pind_val >> ENC_CLK_BIT) & 0x01;
    uint8_t dt  = (pind_val >> ENC_DT_BIT)  & 0x01;
    uint8_t cur = (uint8_t)((clk << 1) | dt);

    g_enc_ab = (uint8_t)(((g_enc_ab << 2) | cur) & 0x0F);
    g_enc_substep += g_enc_states[g_enc_ab];

    if (cur == 0x03) {
        if (g_enc_substep >= 2) {
            g_enc_delta++;
        } else if (g_enc_substep <= -2) {
            g_enc_delta--;
        }
        g_enc_substep = 0;
    }
}

void Encoder_Init(void) {
    ENC_DDR  &= ~((1 << ENC_CLK_BIT) | (1 << ENC_DT_BIT) | (1 << ENC_SW_BIT));
    ENC_PORT |=  ((1 << ENC_CLK_BIT) | (1 << ENC_DT_BIT) | (1 << ENC_SW_BIT));

    uint8_t pind_val = ENC_PIN_REG;
    uint8_t clk = (pind_val >> ENC_CLK_BIT) & 0x01;
    uint8_t dt  = (pind_val >> ENC_DT_BIT)  & 0x01;
    g_enc_ab = (uint8_t)((clk << 1) | dt);
    PCICR  |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT19) | (1 << PCINT20);
}

bool Encoder_Process(uint32_t now_ms,
                     DecoderMode_t* current_mode,
                     DecoderMode_t* last_active_mode,
                     char* current_morse,
                     char* decoded_word) {
    bool display_dirty = false;
    int8_t step = 0;

    uint8_t sreg = SREG;
    cli();
    if (g_enc_delta != 0) {
        step = g_enc_delta;
        g_enc_delta = 0;
    }
    SREG = sreg;

    if (step != 0) {
        const DecoderMode_t working_modes[3] = { MODE_MORSE, MODE_CAESAR, MODE_ATBASH };
        uint8_t idx = 0;
        DecoderMode_t ref = (*current_mode == MODE_WAIT) ? *last_active_mode : *current_mode;

        for (uint8_t i = 0; i < 3U; i++) {
            if (working_modes[i] == ref) {
                idx = i;
                break;
            }
        }

        if (*current_mode == MODE_WAIT) {
            idx = (step > 0) ? 0U : 2U;
        } else {
            idx = (step > 0) ? (uint8_t)((idx + 1U) % 3U) : (uint8_t)((idx + 2U) % 3U);
        }

        *current_mode     = working_modes[idx];
        *last_active_mode = *current_mode;
        current_morse[0]  = '\0';
        g_last_rotate_time = now_ms;

        Buzzer_Beep(30, 1800);
        display_dirty = true;
    }

    bool sw_low = !(ENC_PIN_REG & (1 << ENC_SW_BIT));

    if (sw_low && !g_sw_pressed) {
        g_sw_pressed      = true;
        g_sw_long_handled = false;
        g_sw_press_time   = now_ms;
    } else if (sw_low && g_sw_pressed && !g_sw_long_handled) {
        if ((now_ms - g_sw_press_time) >= ENC_LONG_PRESS_MS) {
            g_sw_long_handled = true;
            *current_mode     = MODE_WAIT;
            current_morse[0]  = '\0';
            decoded_word[0]   = '\0';

            USART0_SendJSON("COMMAND", "CLEAR");
            Buzzer_Beep(60, 1200);
            Timer0_DelayMs(50);
            Buzzer_Beep(80, 800);
            display_dirty = true;
        }
    } else if (!sw_low && g_sw_pressed) {
        uint32_t press_dur = now_ms - g_sw_press_time;
        g_sw_pressed = false;

        if (!g_sw_long_handled && press_dur >= ENC_DEBOUNCE_MS) {
            if (*current_mode == MODE_WAIT) {
                *current_mode    = *last_active_mode;
                current_morse[0] = '\0';
                Buzzer_Beep(45, 1500);
            } else if ((now_ms - g_last_rotate_time) < 2000UL) {
                g_last_rotate_time = 0;
                current_morse[0]   = '\0';
                Buzzer_Beep(45, 1500);
            } else {
                *current_mode    = MODE_WAIT;
                current_morse[0] = '\0';
                Buzzer_Beep(45, 900);
            }
            display_dirty = true;
        }
    }

    return display_dirty;
}
