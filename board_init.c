#include <msp430.h>
#include <driverlib.h>

#include "board.h"
#include "board_init.h"

void board_init_gpio(void)
{
    P1OUT = 0x00;
    P1DIR = 0xFF;
    P2OUT = 0x00;
    P2DIR = 0xFF;
    P3OUT = 0x00;
    P3DIR = 0xFF;
    P4OUT = 0x00;
    P4DIR = 0xFF;

    /*
     * Keep the scaler reset pin as an input with its output latch high.
     * The scaler's reset implementation is unreliable when actively driven.
     */
    GPIO_setAsInputPinWithPullDownResistor(SCALER_RESET_PORT,
                                           SCALER_RESET_PIN);

    GPIO_setAsInputPinWithPullDownResistor(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setAsInputPinWithPullDownResistor(GPIO_PORT_P2, GPIO_PIN6);

    GPIO_setAsInputPinWithPullDownResistor(SCALER_BACKLIGHT_PORT,
                                           SCALER_BACKLIGHT_PIN);

    GPIO_setAsOutputPin(VCCINT_EN_PORT, VCCINT_EN_PIN);
    GPIO_setAsOutputPin(VCC_1V8_EN_PORT, VCC_1V8_EN_PIN);
    GPIO_setAsOutputPin(VCC_2V5_EN_PORT, VCC_2V5_EN_PIN);
    GPIO_setAsOutputPin(VCC_3V3_EN_PORT, VCC_3V3_EN_PIN);
    GPIO_setAsOutputPin(BACKLIGHT_EN_PORT, BACKLIGHT_EN_PIN);
    GPIO_setAsOutputPin(LCD_EN_PORT, LCD_EN_PIN);
    GPIO_setAsOutputPin(MXM_POWER_GOOD_PORT, MXM_POWER_GOOD_PIN);
    GPIO_setAsInputPinWithPullDownResistor(MXM_POWER_ENABLE_PORT,
                                           MXM_POWER_ENABLE_PIN);
    GPIO_setAsOutputPin(MXM_OVERT_PORT, MXM_OVERT_PIN);
    GPIO_HIGH(MXM_OVERT);
    GPIO_setAsOutputPin(FAN_PWM_PORT, FAN_PWM_PIN);
    GPIO_setAsOutputPin(VSA_BLANK_FIX_PORT, VSA_BLANK_FIX_PIN);

    GPIO_setAsInputPinWithPullUpResistor(VCCINT_PGOOD_PORT,
                                         VCCINT_PGOOD_PIN);
    GPIO_setAsInputPinWithPullUpResistor(VCC_1V8_PGOOD_PORT,
                                         VCC_1V8_PGOOD_PIN);
    GPIO_setAsInputPinWithPullUpResistor(VCC_2V5_PGOOD_PORT,
                                         VCC_2V5_PGOOD_PIN);
    GPIO_setAsInputPinWithPullUpResistor(VCC_3V3_PGOOD_PORT,
                                         VCC_3V3_PGOOD_PIN);
    GPIO_setAsOutputPin(FB_D3_PORT, FB_D3_PIN);
    GPIO_setAsOutputPin(FB_D2_PORT, FB_D2_PIN);
    GPIO_setAsInputPinWithPullUpResistor(PCI_RESET_PORT, PCI_RESET_PIN);
    GPIO_setAsInputPinWithPullUpResistor(HOST_CLOCK_PORT, HOST_CLOCK_PIN);
    GPIO_setAsInputPinWithPullUpResistor(HOST_DATA_IN_PORT,
                                         HOST_DATA_IN_PIN);
    GPIO_setAsOutputPin(HOST_DATA_OUT_PORT, HOST_DATA_OUT_PIN);

    PM5CTL0 &= ~LOCKLPM5;
    GPIO_LOW(BACKLIGHT_EN);
}
