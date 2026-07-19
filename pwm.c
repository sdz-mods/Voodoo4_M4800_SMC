#include <msp430.h>
#include <stdint.h>

#include "firmware_config.h"
#include "pwm.h"

static volatile uint16_t backlight_duty_cycle = 10;
static uint32_t fan_startup_delay = FAN_STARTUP_DELAY_TICKS;
static volatile uint16_t fan_duty_cycle = 3;
static uint16_t fan_counter;
static uint16_t backlight_counter;
static uint16_t backlight_prescale;

void pwm_set_backlight_percent(uint16_t percent)
{
    /*
     * Hard floor at 10%: a corrupted host write (or any other path) must never
     * be able to drive the backlight fully dark and leave the user with a black
     * panel and no way to see the recovery UI. This is the final guard right at
     * the point the duty is programmed, independent of the stored-setting clamp.
     */
    if (percent < 10)
        percent = 10;
    if (percent > 100)
        percent = 100;
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

    /*
     * Backlight runs at a fraction of the ISR rate (prescale) so its low-duty
     * on-pulse is many ISR periods long and immune to jitter that would
     * otherwise stretch a one-ISR-period pulse. The fan above stays at the full
     * ISR rate because it stalls at low PWM frequencies.
     */
    if (++backlight_prescale >= BACKLIGHT_PWM_PRESCALE) {
        backlight_prescale = 0;
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
}
