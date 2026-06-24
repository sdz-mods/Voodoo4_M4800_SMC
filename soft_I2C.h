#ifndef SOFT_I2C_H
#define SOFT_I2C_H

#include <stdint.h>

uint8_t sI2C_read1b(uint8_t address, uint8_t reg);
uint8_t sI2C_write_reg(uint8_t address, uint8_t reg, uint8_t value);
uint8_t sI2C_write_byte(uint8_t address, uint8_t value);

#endif
