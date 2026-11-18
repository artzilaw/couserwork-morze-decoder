#include "adc_sensors.h"

static uint32_t g_last_sound_time = 0;
static bool     g_raw_prev_state  = false;
static bool     g_filtered_state  = false;
static uint32_t g_state_change_ms = 0;

void Sensors_Init(void) {
    BTN_MORSE_DDR  &= ~(1 << BTN_MORSE_BIT);
    BTN_MORSE_PORT |=  (1 << BTN_MORSE_BIT);


    DDRC  &= ~((1 << PC0) | (1 << PC1));
    PORTC &= ~((1 << PC0) | (1 << PC1));
    DIDR0 |= (1 << ADC0D) | (1 << ADC1D);

    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_Read(uint8_t channel) {
    ADMUX = (uint8_t)((ADMUX & 0xF0) | (channel & 0x0F));
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {
    }
    return ADC;
}

static bool Sensors_ReadRawChannels(uint32_t now_ms) {
    if (!(BTN_MORSE_PIN_REG & (1 << BTN_MORSE_BIT))) {
        return true;
    }

    if (ADC_Read(ADC_CH_PHOTO) > LIGHT_THRESHOLD) {
        return true;
    }

    if (ADC_Read(ADC_CH_SOUND) > SOUND_THRESHOLD) {
        g_last_sound_time = now_ms;
        return true;
    }
    if ((now_ms - g_last_sound_time) < SOUND_HOLD_MS) {
        return true;
    }

    return false;
}

bool Sensors_CheckSignal(uint32_t current_time_ms) {
    bool raw_active = Sensors_ReadRawChannels(current_time_ms);

    if (raw_active != g_raw_prev_state) {
        g_raw_prev_state  = raw_active;
        g_state_change_ms = current_time_ms;
    }

    if ((current_time_ms - g_state_change_ms) >= NOISE_FILTER_MS) {
        g_filtered_state = raw_active;
    }

    return g_filtered_state;
}
