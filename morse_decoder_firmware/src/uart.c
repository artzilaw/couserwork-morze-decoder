#include "uart.h"

#define UART_RX_BUF_SIZE 32

static volatile char g_rx_buf[UART_RX_BUF_SIZE];
static volatile uint8_t g_rx_head = 0;
static volatile uint8_t g_rx_tail = 0;

ISR(USART_RX_vect) {
    char incoming = UDR0;
    uint8_t next_head = (uint8_t)((g_rx_head + 1U) % UART_RX_BUF_SIZE);
    if (next_head != g_rx_tail) {
        g_rx_buf[g_rx_head] = incoming;
        g_rx_head = next_head;
    }
}

void USART0_Init(void) {
    uint16_t ubrr = (uint16_t)((F_CPU / (16UL * UART_BAUD_RATE)) - 1UL);
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr & 0xFF);

    UCSR0A = 0x00;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void USART0_SendChar(char c) {
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = c;
}

void USART0_SendString(const char* str) {
    while (*str != '\0') {
        USART0_SendChar(*str++);
    }
}

void USART0_SendLine(const char* str) {
    USART0_SendString(str);
    USART0_SendChar('\r');
    USART0_SendChar('\n');
}

void USART0_SendJSON(const char* mode, const char* ch) {
    USART0_SendString("{\"mode\":\"");
    USART0_SendString(mode);
    USART0_SendString("\",\"char\":\"");
    USART0_SendString(ch);
    USART0_SendLine("\"}");
}

bool USART0_Available(void) {
    return (g_rx_head != g_rx_tail);
}

char USART0_ReadChar(void) {
    if (g_rx_head == g_rx_tail) {
        return '\0';
    }
    char c = g_rx_buf[g_rx_tail];
    g_rx_tail = (uint8_t)((g_rx_tail + 1U) % UART_RX_BUF_SIZE);
    return c;
}
