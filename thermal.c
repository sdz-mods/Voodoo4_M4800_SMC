#include <msp430.h>
#include <stdint.h>

#include "firmware_config.h"
#include "soft_I2C.h"
#include "thermal.h"

#if TMP103_VARIANT == TMP103_VARIANT_A
#define TMP103_I2C_ADDR 0x70
#elif TMP103_VARIANT == TMP103_VARIANT_B
#define TMP103_I2C_ADDR 0x71
#elif TMP103_VARIANT == TMP103_VARIANT_C
#define TMP103_I2C_ADDR 0x72
#else
#error Unsupported TMP103 variant
#endif

#define TMP103_TEMP_REG 0x00
#define CALADC_15V_30C *((uint16_t *)0x1A1A)
#define CALADC_15V_85C *((uint16_t *)0x1A1C)

typedef struct {
    uint8_t max_temp;
    uint8_t duty;
} fan_curve_entry_t;

static const fan_curve_entry_t fan_curve[] = {
    { 30, 1 },
    { 40, 3 },
    { 50, 5 },
    { 60, 6 },
    { 70, 7 },
    { 75, 8 },
    { 80, 9 },
    { OVERTEMP_SHUTDOWN_C, 10 },
};

static uint8_t fan_duty_for_temperature(uint8_t temperature)
{
    uint8_t i;

    if (temperature == 0 || temperature == 255)
        return 10;
    if (temperature >= EXTERNAL_TEMP_INVALID_MIN)
        return 0;
    for (i = 0; i < sizeof(fan_curve) / sizeof(fan_curve[0]); i++) {
        if (temperature <= fan_curve[i].max_temp)
            return fan_curve[i].duty;
    }
    return 10;
}

static int32_t read_internal_temperature(void)
{
    int32_t adc_value;

    ADCIFG &= ~ADCIFG0;
    ADCCTL0 |= ADCENC | ADCSC;
    while ((ADCIFG & ADCIFG0) == 0)
        ;
    adc_value = ADCMEM0;
    return (adc_value - (int32_t)CALADC_15V_30C) * (85 - 30) /
        ((int32_t)CALADC_15V_85C - (int32_t)CALADC_15V_30C) + 30;
}

void thermal_read(thermal_sample_t *sample)
{
    sample->external_temp_c =
        sI2C_read1b(TMP103_I2C_ADDR, TMP103_TEMP_REG);
    sample->fan_duty =
        fan_duty_for_temperature(sample->external_temp_c);
    sample->internal_temp_c = read_internal_temperature();
}
