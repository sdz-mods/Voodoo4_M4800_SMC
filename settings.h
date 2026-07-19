#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

#include "fir_gen.h"   /* FIR_FAM_COUNT and family enum */

#define BACKLIGHT_MAX_PERCENT 100
#define BACKLIGHT_MIN_PERCENT 10   /* never below this: keep the panel visible */
#define VCORE_DEFAULT_SETTING 1
#define VCORE_MAX_SETTING 6
#define FB_SIZE_DEFAULT_SETTING 0

/* XIO register indices (host write = [index, value]; index MUST be < 0x80). */
#define REG_BACKLIGHT       0x00
#define REG_VCORE           0x01
#define REG_FBSIZE          0x02
#define REG_VSA_BLANK_FIX   0x03
#define REG_DOS43           0x10
#define REG_SHARPNESS       0x11
#define REG_CONTRAST        0x12
#define REG_PEAKING         0x13
#define REG_RGB_R           0x14
#define REG_RGB_G           0x15
#define REG_RGB_B           0x16
#define REG_FILTER_FAMILY   0x17   /* scale-up FIR family (fir_gen.h FIR_FAM_*) */
#define REG_FILTER_P1       0x18   /* filter param 1, step index (per family)   */
#define REG_FILTER_P2       0x19   /* filter param 2, step index (Mitchell C)   */

/* which hardware domain a register write affects (returned to the caller) */
typedef enum {
    REG_DOMAIN_NONE = 0,
    REG_DOMAIN_POWER,   /* backlight / vcore / fbsize / blank-fix */
    REG_DOMAIN_SCALER,  /* dos43 / contrast / peaking / rgb (single-byte push) */
    REG_DOMAIN_FILTER   /* filter family/p1/p2 -> regenerate + LOAD_FIR push    */
} reg_domain_t;

/* existing power/card settings */
extern uint16_t backlight_percent;
extern uint16_t vcore_setting;
extern uint16_t fbsize_setting;
extern uint16_t vsa_blank_fix_en;

/* scaler settings (pushed to the RTD over UART; defaults match the scaler) */
extern uint16_t scaler_dos43;      /* 0/1,   default 1 */
extern uint16_t scaler_sharpness;  /* legacy 0..4 (unused; superseded by filter) */
extern uint16_t scaler_contrast;   /* 0..100, default 40 */
extern uint16_t scaler_peaking;    /* 0..30,  default 0 (edge enhance) */
extern uint16_t scaler_rgb_r;      /* 0..100, default 50 */
extern uint16_t scaler_rgb_g;      /* 0..100, default 50 */
extern uint16_t scaler_rgb_b;      /* 0..100, default 50 */

/*
 * Scale-up FIR filter, generated on-MSP from small params and pushed as a
 * 128-byte table via LOAD_FIR. p1/p2 are per-family step indices (see fir_gen.h)
 * and are stored PER FAMILY so each filter keeps its own tuning across reboots.
 * The active filter is scaler_filter_p1[scaler_filter_family] etc.
 * Defaults: Mitchell B=0.40/C=0.55, Keys a=-0.45, Gaussian sigma=0.50.
 */
extern uint16_t scaler_filter_family;             /* FIR_FAM_*, default Mitchell */
extern uint16_t scaler_filter_p1[FIR_FAM_COUNT];  /* per-family param-1 step idx */
extern uint16_t scaler_filter_p2[FIR_FAM_COUNT];  /* per-family param-2 step idx */

void settings_sanitize(void);

/*
 * Apply one indexed register write to the FRAM setting variables (value is
 * range-clamped). Returns which hardware domain changed so the caller can
 * apply it. Must be called with data-FRAM write protection disabled.
 */
reg_domain_t settings_write_register(uint8_t index, uint8_t value);

#endif
