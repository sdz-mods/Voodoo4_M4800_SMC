#include <msp430.h>
#include <stdint.h>

#include "board.h"
#include "firmware_config.h"
#include "host_protocol.h"
#include "monitor.h"
#include "power.h"
#include "pwm.h"
#include "scaler_uart.h"
#include "settings.h"
#include "thermal.h"

static uint8_t external_temp_c;
static int32_t internal_temp_c;
static volatile uint8_t monitor_due = 1;

enum fault_reason {
    FAULT_VCCINT_PGOOD = 1,
    FAULT_1V8_PGOOD,
    FAULT_2V5_PGOOD,
    FAULT_3V3_PGOOD,
    FAULT_EXTERNAL_OVERTEMP,
    FAULT_INTERNAL_OVERTEMP,
};

static void fault_latch(enum fault_reason reason)
{
    pwm_set_fan_duty(reason);
    power_latch_off();
}

void monitor_update_host_response(void)
{
    uint8_t response[HOST_TX_BYTES];
    const scaler_status_t *s = scaler_uart_status();

    /*
     * Live data first so a short "status" read (MXM_STATUS_BYTES) gets the
     * scaler link/lock/resolution quickly; settings follow.
     */
    response[0]  = HOST_PROTO_VERSION;
    response[1]  = (s->link_ok ? 0x01 : 0x00) | (s->lock ? 0x02 : 0x00);
    response[2]  = (uint8_t)(s->in_width & 0xFF);
    response[3]  = (uint8_t)(s->in_width >> 8);
    response[4]  = (uint8_t)(s->in_height & 0xFF);
    response[5]  = (uint8_t)(s->in_height >> 8);
    response[6]  = external_temp_c;
    response[7]  = internal_temp_c;
    response[8]  = pwm_get_fan_duty() * 10;
    response[9]  = backlight_percent;
    response[10] = vcore_setting;
    response[11] = fbsize_setting;
    response[12] = vsa_blank_fix_en;
    response[13] = scaler_dos43;
    response[14] = scaler_sharpness;
    response[15] = scaler_contrast;
    response[16] = scaler_peaking;
    response[17] = scaler_rgb_r;
    response[18] = scaler_rgb_g;
    response[19] = scaler_rgb_b;
    host_protocol_set_response(response);
}

uint8_t monitor_is_due(void)
{
    return monitor_due;
}

void monitor_run(void)
{
    thermal_sample_t sample;

    thermal_read(&sample);
    external_temp_c = sample.external_temp_c;
    internal_temp_c = sample.internal_temp_c;
    pwm_set_fan_duty(sample.fan_duty);

    if (GPIO_READ(VCCINT_PGOOD) == 0)
        fault_latch(FAULT_VCCINT_PGOOD);
    if (GPIO_READ(VCC_1V8_PGOOD) == 0)
        fault_latch(FAULT_1V8_PGOOD);
    if (GPIO_READ(VCC_2V5_PGOOD) == 0)
        fault_latch(FAULT_2V5_PGOOD);
    if (GPIO_READ(VCC_3V3_PGOOD) == 0)
        fault_latch(FAULT_3V3_PGOOD);
    if (external_temp_c > OVERTEMP_SHUTDOWN_C &&
        external_temp_c <= EXTERNAL_TEMP_INVALID_MIN)
        fault_latch(FAULT_EXTERNAL_OVERTEMP);
    if (internal_temp_c > OVERTEMP_SHUTDOWN_C)
        fault_latch(FAULT_INTERNAL_OVERTEMP);

    /* refresh cached scaler live status (reply parsed async in the RX ISR) */
    scaler_uart_request_status();

    monitor_due = 0;
    monitor_update_host_response();
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer1_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(TIMER1_A0_VECTOR))) Timer1_A0_ISR(void)
#else
#error Compiler not supported!
#endif
{
    monitor_due = 1;
    host_protocol_timer_tick();
}
