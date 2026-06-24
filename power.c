#include <msp430.h>

#include "board.h"
#include "power.h"

void power_enable_rails(void)
{
    GPIO_HIGH(VCCINT_EN);
    __delay_cycles(10);
    GPIO_HIGH(VCC_1V8_EN);
    __delay_cycles(10);
    GPIO_HIGH(VCC_2V5_EN);
    __delay_cycles(10);
    GPIO_HIGH(VCC_3V3_EN);
}

void power_latch_off(void)
{
    GPIO_LOW(VCCINT_EN);
    GPIO_LOW(VCC_1V8_EN);
    GPIO_LOW(VCC_2V5_EN);
    GPIO_LOW(VCC_3V3_EN);

    while (1)
        ;
}
