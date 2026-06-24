#include <msp430.h>
#include <stdint.h>

#include "board.h"
#include "board_init.h"
#include "card_control.h"
#include "display.h"
#include "firmware_config.h"
#include "hardware_init.h"
#include "host_protocol.h"
#include "monitor.h"
#include "power.h"
#include "pwm.h"
#include "settings.h"

static void delay_legacy_units(uint16_t units)
{
    uint16_t j;
    for (j = 0; j * 16 < units; j++)
        __delay_cycles(1000 * 8);
}

static void delay_ms(uint16_t ms)
{
    while (ms--)
        __delay_cycles(16000);
}

void main(void)
{
    host_request_t request;

    WDTCTL = WDTPW | WDTHOLD;
    board_init_gpio();
    host_protocol_init();

    hardware_init_clocks_timers_adc();
    SYSCFG0 = FRWPPW | DFWP;
    settings_sanitize();
    pwm_set_backlight_percent(backlight_percent);
    SYSCFG0 = FRWPPW | PFWP | DFWP;
    monitor_update_host_response();
    card_apply_framebuffer_straps();
    card_apply_vsa_blank_fix();

    GPIO_HIGH(MXM_OVERT);
    __delay_cycles(10);
    power_enable_rails();

    GPIO_HIGH(MXM_POWER_GOOD);
    GPIO_HIGH(LCD_EN);
    GPIO_HIGH(FAN_PWM);
    GPIO_HIGH(SCALER_RESET);
    GPIO_LOW(BACKLIGHT_EN);
    delay_ms(BACKLIGHT_ENABLE_DELAY_MS);
    GPIO_HIGH(BACKLIGHT_EN);

    delay_legacy_units(PRE_VCORE_DELAY_LEGACY_UNITS);
    __enable_interrupt();
    card_apply_vcore();
    delay_legacy_units(PRE_BRIDGE_DELAY_LEGACY_UNITS);
    display_bridge_init();

    while (1) {
        host_protocol_poll_timeout();

        if (host_protocol_take_request(&request)) {
            SYSCFG0 = FRWPPW | DFWP;
            settings_decode_host(request.backlight, request.settings);
            pwm_set_backlight_percent(backlight_percent);
            SYSCFG0 = FRWPPW | PFWP | DFWP;
            card_apply_vsa_blank_fix();
            card_apply_vcore();
        }

        /* Apply reset-safe settings again during external PCI reset. */
        if (GPIO_READ(PCI_RESET) == 0) {
            card_apply_framebuffer_straps();
            card_apply_vsa_blank_fix();
        }

        if (monitor_is_due() && host_protocol_is_idle())
            monitor_run();
    }
}
