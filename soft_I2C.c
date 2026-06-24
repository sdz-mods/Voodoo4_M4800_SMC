#include <msp430.h>
#include <stdint.h>

#include "firmware_config.h"
#include "soft_I2C.h"

#define SOFT_I2C_SCL BIT6
#define SOFT_I2C_SDA BIT2

static void scl_release(void)
{
    P3DIR &= ~SOFT_I2C_SCL;
}

static void scl_high(void)
{
    scl_release();
}

static void scl_low(void)
{
    P3OUT &= ~SOFT_I2C_SCL;
    P3DIR |= SOFT_I2C_SCL;
}

static void sda_release(void)
{
    P3DIR &= ~SOFT_I2C_SDA;
}

static void sda_low(void)
{
    P3OUT &= ~SOFT_I2C_SDA;
    P3DIR |= SOFT_I2C_SDA;
}

static uint8_t sda_level(void)
{
    return (P3IN & SOFT_I2C_SDA) != 0;
}

static void i2c_wait(void)
{
    __delay_cycles(SOFT_I2C_DELAY_CYCLES);
}

static void i2c_prepare(void)
{
    P3SEL0 &= ~(SOFT_I2C_SCL | SOFT_I2C_SDA);
    P3SEL1 &= ~(SOFT_I2C_SCL | SOFT_I2C_SDA);
    P3REN &= ~(SOFT_I2C_SCL | SOFT_I2C_SDA);
    P3OUT &= ~(SOFT_I2C_SCL | SOFT_I2C_SDA);
    scl_release();
    sda_release();
    i2c_wait();
}

static void i2c_start(void)
{
    sda_release();
    scl_high();
    i2c_wait();
    sda_low();
    i2c_wait();
    scl_low();
    i2c_wait();
}

static void i2c_stop(void)
{
    sda_low();
    scl_low();
    i2c_wait();
    scl_high();
    i2c_wait();
    sda_release();
    i2c_wait();
}

static uint8_t i2c_write_byte(uint8_t byte)
{
    uint8_t bit;
    uint8_t nack;

    for (bit = 0; bit < 8; bit++) {
        if (byte & 0x80)
            sda_release();
        else
            sda_low();
        byte <<= 1;
        i2c_wait();
        scl_high();
        i2c_wait();
        scl_low();
        i2c_wait();
    }

    sda_release();
    i2c_wait();
    scl_high();
    i2c_wait();
    nack = sda_level();
    scl_low();
    i2c_wait();
    return nack;
}

static uint8_t i2c_read_byte(void)
{
    uint8_t bit;
    uint8_t value = 0;

    sda_release();
    for (bit = 0; bit < 8; bit++) {
        i2c_wait();
        scl_high();
        value = (value << 1) | sda_level();
        i2c_wait();
        scl_low();
    }

    /* NACK the final byte before STOP. */
    sda_release();
    i2c_wait();
    scl_high();
    i2c_wait();
    scl_low();
    i2c_wait();
    return value;
}

uint8_t sI2C_read1b(uint8_t address, uint8_t reg)
{
    uint8_t value = 0xff;

    i2c_prepare();
    i2c_start();
    if (i2c_write_byte(address << 1))
        goto stop;
    if (i2c_write_byte(reg))
        goto stop;

    /* Repeated START keeps the selected register pointer active. */
    i2c_start();
    if (i2c_write_byte((address << 1) | 1))
        goto stop;
    value = i2c_read_byte();

stop:
    i2c_stop();
    return value;
}

uint8_t sI2C_write_reg(uint8_t address, uint8_t reg, uint8_t value)
{
    uint8_t nack;

    i2c_prepare();
    i2c_start();
    nack = i2c_write_byte(address << 1);
    nack |= i2c_write_byte(reg);
    nack |= i2c_write_byte(value);
    i2c_stop();
    return nack;
}

uint8_t sI2C_write_byte(uint8_t address, uint8_t value)
{
    uint8_t nack;

    i2c_prepare();
    i2c_start();
    nack = i2c_write_byte(address << 1);
    nack |= i2c_write_byte(value);
    i2c_stop();
    return nack;
}
