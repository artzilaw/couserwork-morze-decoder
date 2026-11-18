#include "twi_i2c.h"

void TWI_Init(void) {
    DDRC  &= ~((1 << PC4) | (1 << PC5));
    PORTC |=  (1 << PC4) | (1 << PC5);


    TWSR = 0x00;
    TWBR = 72;
    TWCR = (1 << TWEN);
}

void TWI_StartWrite(uint8_t slave_addr) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT))) {
    }

    TWDR = (uint8_t)(slave_addr << 1);
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT))) {
    }
}

void TWI_WriteByte(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT))) {
    }
}

void TWI_Stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}
