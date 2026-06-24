#include <msp430.h>
#include <stdint.h>

#include "firmware_config.h"
#include "pwm.h"

static volatile uint16_t backlight_duty_cycle = 10;
static uint32_t fan_startup_delay = FAN_STARTUP_DELAY_TICKS;
static volatile uint16_t fan_duty_cycle = 3;
static uint16_t fan_counter;
static uint16_t backlight_counter;

void pwm_set_backlight_percent(uint16_t percent)
{
    backlight_duty_cycle = percent / 10;
}

void pwm_set_fan_duty(uint16_t duty)
{
    fan_duty_cycle = duty;
}

uint16_t pwm_get_fan_duty(void)
{
    return fan_duty_cycle;
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR(void)
#else
#error Compiler not supported!
#endif
{
    if (fan_startup_delay > 0) {
        fan_startup_delay--;
    } else {
        if (fan_counter <= fan_duty_cycle)
            P3OUT |= BIT1;
        else
            P3OUT &= ~BIT1;
        if (fan_counter >= 10)
            fan_counter = 0;
        fan_counter++;
    }

#if ENABLE_SCALER_BACKLIGHT_GATE
    if ((P4IN & BIT2) && backlight_counter <= backlight_duty_cycle)
#else
    if (backlight_counter <= backlight_duty_cycle)
#endif
        P2OUT |= BIT0;
    else
        P2OUT &= ~BIT0;
    if (backlight_counter >= 10)
        backlight_counter = 0;
    backlight_counter++;
}
