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
#include "scaler_uart.h"
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

/* main-loop iterations (~1 ms each) the host must be quiet before the deferred
 * scale-up FIR is pushed to the scaler. Longer than any inter-write gap. */
#define FILTER_FLUSH_QUIET_MS 50

void main(void)
{
    host_request_t request;
    uint16_t filter_quiet = 0;

    WDTCTL = WDTPW | WDTHOLD;
    board_init_gpio();
    host_protocol_init();

    hardware_init_clocks_timers_adc();
    scaler_uart_init();
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

    /* sync the scaler to our stored settings now that it has booted */
    scaler_uart_push_all();

    while (1) {
        host_protocol_poll_timeout();

        if (host_protocol_take_request(&request)) {
            filter_quiet = 0;      /* host active: defer the FIR flush */
            if (request.reg == HOST_CMD_PREFIX) {
                /* side-band command, not a register store */
                if (request.value == HOST_CMD_SCALER_DOS_ASPECT)
                    scaler_uart_set_dos_override(0);  /* restore stored dos43 */
                else if (request.value == HOST_CMD_SCALER_FULLSCREEN)
                    scaler_uart_set_dos_override(1);  /* force DOS stretch */
            } else {
                reg_domain_t domain;

                SYSCFG0 = FRWPPW | DFWP;            /* unlock data FRAM */
                domain = settings_write_register(request.reg, request.value);
                SYSCFG0 = FRWPPW | PFWP | DFWP;     /* relock */

                if (domain == REG_DOMAIN_POWER) {
                    pwm_set_backlight_percent(backlight_percent);
                    card_apply_vsa_blank_fix();
                    card_apply_vcore();
                } else if (domain == REG_DOMAIN_SCALER) {
                    scaler_uart_send_register(request.reg);
                } else if (domain == REG_DOMAIN_FILTER) {
                    /* defer the ~67 ms FIR push (flushed once the host goes
                     * quiet, below) so an Apply burst stays responsive */
                    scaler_uart_mark_filter_dirty();
                }
            }

            /* reflect the write in the read block immediately */
            monitor_update_host_response();
        } else if (scaler_uart_filter_pending() && host_protocol_is_idle()) {
            /*
             * Coalesced FIR flush: only push once the host has been quiet for
             * ~FILTER_FLUSH_QUIET_MS, so the many filter writes of a single
             * Apply collapse into ONE 67 ms transfer instead of one per write.
             */
            if (++filter_quiet >= FILTER_FLUSH_QUIET_MS) {
                filter_quiet = 0;
                scaler_uart_flush_filter();
                monitor_update_host_response();
            } else {
                delay_ms(1);
            }
        } else {
            filter_quiet = 0;      /* a transfer is in progress: keep waiting */
        }

        /* Apply reset-safe settings again during external PCI reset. */
        if (GPIO_READ(PCI_RESET) == 0) {
            card_apply_framebuffer_straps();
            card_apply_vsa_blank_fix();
            /* a reboot re-enters SeaBIOS: force DOS stretch again (deduped) */
            scaler_uart_set_dos_override(1);
        }

        if (monitor_is_due() && host_protocol_is_idle())
            monitor_run();
    }
}
