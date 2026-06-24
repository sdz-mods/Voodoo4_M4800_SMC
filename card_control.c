#include <msp430.h>

#include "board.h"
#include "card_control.h"
#include "settings.h"
#include "soft_I2C.h"

void card_apply_vcore(void)
{
    static const uint8_t vcore_values[] = {
        0x1d, 0x33, 0x44, 0x53, 0x5e, 0x69, 0x71
    };

    if (vcore_setting > VCORE_MAX_SETTING)
        vcore_setting = VCORE_DEFAULT_SETTING;
    sI2C_write_byte(0x2e, vcore_values[vcore_setting]);
}

void card_apply_framebuffer_straps(void)
{
    if (fbsize_setting == 1) {
        GPIO_LOW(FB_D3);
        GPIO_HIGH(FB_D2);
    } else {
        GPIO_HIGH(FB_D3);
        GPIO_LOW(FB_D2);
    }
}

void card_apply_vsa_blank_fix(void)
{
    P2DIR |= BIT3;
    if (vsa_blank_fix_en)
        P2OUT |= BIT3;
    else
        P2OUT &= ~BIT3;
}
