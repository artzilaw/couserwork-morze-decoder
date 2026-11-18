#include "timer.h"

static volatile uint32_t g_millis_counter = 0;

ISR(TIMER0_COMPA_vect) {
    g_millis_counter++;
}

ISR(TIMER2_COMPA_vect) {
    BUZZER_PORT ^= (1 << BUZZER_BIT);
}

void Timer0_Init(void) {
    BUZZER_DDR  |= (1 << BUZZER_BIT);
    BUZZER_PORT &= ~(1 << BUZZER_BIT);


    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00); 
    TCNT0  = 0;
    OCR0A  = 249;
    TIMSK0 = (1 << OCIE0A);             
}

uint32_t Timer0_Millis(void) {
    uint32_t m;
    uint8_t sreg = SREG;
    cli();
    m = g_millis_counter;
    SREG = sreg;
    return m;
}

void Timer0_DelayMs(uint16_t ms) {
    uint32_t start = Timer0_Millis();
    while ((Timer0_Millis() - start) < (uint32_t)ms) {}
}

void Timer2_Tone(uint16_t freq_hz) {
    if (freq_hz < 100U) {
        Timer2_NoTone();
        return;
    }


    uint32_t ocr = (62500UL / (uint32_t)freq_hz);
    if (ocr > 0) {
        ocr -= 1;
    }
    if (ocr > 255UL) {
        ocr = 255UL;
    }

    TCCR2A = (1 << WGM21);
    OCR2A  = (uint8_t)ocr;
    TIMSK2 = (1 << OCIE2A);
    TCCR2B = (1 << CS22) | (1 << CS20);
}

void Timer2_NoTone(void) {
    TCCR2B = 0;
    TIMSK2 &= ~(1 << OCIE2A);
    BUZZER_PORT &= ~(1 << BUZZER_BIT);
}

void Buzzer_Beep(uint16_t duration_ms, uint16_t freq_hz) {
    Timer2_Tone(freq_hz);
    Timer0_DelayMs(duration_ms);
    Timer2_NoTone();
}
