#include <stdint.h>

#include "settings.h"
#include "fir_gen.h"

/*
 * Keep this compatibility slot first. Existing cards store the remaining
 * settings at the following fixed FRAM addresses. New settings are APPENDED
 * after the original block so existing stored values keep their addresses.
 */
#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(persistent_layout_v1_reserved)
uint16_t persistent_layout_v1_reserved = 10;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t persistent_layout_v1_reserved = 10;
#else
uint16_t persistent_layout_v1_reserved = 10;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(backlight_percent)
uint16_t backlight_percent = 100;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t backlight_percent = 100;
#else
uint16_t backlight_percent = 100;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(vcore_setting)
uint16_t vcore_setting = VCORE_DEFAULT_SETTING;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t vcore_setting = VCORE_DEFAULT_SETTING;
#else
uint16_t vcore_setting = VCORE_DEFAULT_SETTING;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(fbsize_setting)
uint16_t fbsize_setting = FB_SIZE_DEFAULT_SETTING;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t fbsize_setting = FB_SIZE_DEFAULT_SETTING;
#else
uint16_t fbsize_setting = FB_SIZE_DEFAULT_SETTING;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(vsa_blank_fix_en)
uint16_t vsa_blank_fix_en = 0;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t vsa_blank_fix_en = 0;
#else
uint16_t vsa_blank_fix_en = 0;
#endif

/* ---- scaler settings (appended; defaults match the RTD firmware) -------- */

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_dos43)
uint16_t scaler_dos43 = 1;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_dos43 = 1;
#else
uint16_t scaler_dos43 = 1;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_sharpness)
uint16_t scaler_sharpness = 2;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_sharpness = 2;
#else
uint16_t scaler_sharpness = 2;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_contrast)
uint16_t scaler_contrast = 40;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_contrast = 40;
#else
uint16_t scaler_contrast = 40;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_peaking)
uint16_t scaler_peaking = 0;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_peaking = 0;
#else
uint16_t scaler_peaking = 0;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_rgb_r)
uint16_t scaler_rgb_r = 50;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_rgb_r = 50;
#else
uint16_t scaler_rgb_r = 50;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_rgb_g)
uint16_t scaler_rgb_g = 50;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_rgb_g = 50;
#else
uint16_t scaler_rgb_g = 50;
#endif

#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_rgb_b)
uint16_t scaler_rgb_b = 50;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_rgb_b = 50;
#else
uint16_t scaler_rgb_b = 50;
#endif

/* scale-up FIR filter params (default = Mitchell B=0.40 / C=0.55) */
#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_filter_family)
uint16_t scaler_filter_family = FIR_FAM_MITCHELL;
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_filter_family = FIR_FAM_MITCHELL;
#else
uint16_t scaler_filter_family = FIR_FAM_MITCHELL;
#endif

/* per-family param 1 (Mitchell B / Keys a / Gauss sigma / Lanczos unused) */
#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_filter_p1)
uint16_t scaler_filter_p1[FIR_FAM_COUNT] = { 8, 11, 4, 0 };
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_filter_p1[FIR_FAM_COUNT] = { 8, 11, 4, 0 };
#else
uint16_t scaler_filter_p1[FIR_FAM_COUNT] = { 8, 11, 4, 0 };
#endif

/* per-family param 2 (Mitchell C only) */
#ifdef __TI_COMPILER_VERSION__
#pragma PERSISTENT(scaler_filter_p2)
uint16_t scaler_filter_p2[FIR_FAM_COUNT] = { 11, 0, 0, 0 };
#elif __IAR_SYSTEMS_ICC__
__persistent uint16_t scaler_filter_p2[FIR_FAM_COUNT] = { 11, 0, 0, 0 };
#else
uint16_t scaler_filter_p2[FIR_FAM_COUNT] = { 11, 0, 0, 0 };
#endif

void settings_sanitize(void)
{
    if (backlight_percent > BACKLIGHT_MAX_PERCENT)
        backlight_percent = BACKLIGHT_MAX_PERCENT;
    if (backlight_percent < BACKLIGHT_MIN_PERCENT)
        backlight_percent = BACKLIGHT_MIN_PERCENT;
    if (vcore_setting > VCORE_MAX_SETTING)
        vcore_setting = VCORE_DEFAULT_SETTING;
    if (fbsize_setting > 1)
        fbsize_setting = FB_SIZE_DEFAULT_SETTING;
    vsa_blank_fix_en = !!vsa_blank_fix_en;

    scaler_dos43 = !!scaler_dos43;
    if (scaler_sharpness > 4)
        scaler_sharpness = 2;
    if (scaler_contrast > 100)
        scaler_contrast = 40;
    if (scaler_peaking > 30)
        scaler_peaking = 0;
    if (scaler_rgb_r > 100)
        scaler_rgb_r = 50;
    if (scaler_rgb_g > 100)
        scaler_rgb_g = 50;
    if (scaler_rgb_b > 100)
        scaler_rgb_b = 50;

    if (scaler_filter_family >= FIR_FAM_COUNT)
        scaler_filter_family = FIR_FAM_MITCHELL;
    {
        uint8_t f;
        for (f = 0; f < FIR_FAM_COUNT; f++) {
            if (scaler_filter_p1[f] > fir_p1_max(f))
                scaler_filter_p1[f] = fir_p1_max(f);
            if (scaler_filter_p2[f] > fir_p2_max(f))
                scaler_filter_p2[f] = fir_p2_max(f);
        }
    }
}

reg_domain_t settings_write_register(uint8_t index, uint8_t value)
{
    switch (index) {
    case REG_BACKLIGHT:
        backlight_percent = value;
        settings_sanitize();
        return REG_DOMAIN_POWER;
    case REG_VCORE:
        vcore_setting = value;
        settings_sanitize();
        return REG_DOMAIN_POWER;
    case REG_FBSIZE:
        fbsize_setting = value;
        settings_sanitize();
        return REG_DOMAIN_POWER;
    case REG_VSA_BLANK_FIX:
        vsa_blank_fix_en = value;
        settings_sanitize();
        return REG_DOMAIN_POWER;

    /*
     * Scaler settings are idempotent: if the value is unchanged, return
     * REG_DOMAIN_NONE so the caller does NOT re-send it to the scaler. This
     * avoids a needless mode re-lock (black screen) on the dos43 path when an
     * Apply carries values identical to what is already stored.
     */
    case REG_DOS43: {
        uint16_t v = value ? 1 : 0;
        if (scaler_dos43 == v)
            return REG_DOMAIN_NONE;
        scaler_dos43 = v;
        return REG_DOMAIN_SCALER;
    }
    case REG_SHARPNESS: {
        uint16_t v = (value > 4) ? 4 : value;
        if (scaler_sharpness == v)
            return REG_DOMAIN_NONE;
        scaler_sharpness = v;
        return REG_DOMAIN_SCALER;
    }
    case REG_CONTRAST: {
        uint16_t v = (value > 100) ? 100 : value;
        if (scaler_contrast == v)
            return REG_DOMAIN_NONE;
        scaler_contrast = v;
        return REG_DOMAIN_SCALER;
    }
    case REG_PEAKING: {
        uint16_t v = (value > 30) ? 30 : value;
        if (scaler_peaking == v)
            return REG_DOMAIN_NONE;
        scaler_peaking = v;
        return REG_DOMAIN_SCALER;
    }
    case REG_RGB_R: {
        uint16_t v = (value > 100) ? 100 : value;
        if (scaler_rgb_r == v)
            return REG_DOMAIN_NONE;
        scaler_rgb_r = v;
        return REG_DOMAIN_SCALER;
    }
    case REG_RGB_G: {
        uint16_t v = (value > 100) ? 100 : value;
        if (scaler_rgb_g == v)
            return REG_DOMAIN_NONE;
        scaler_rgb_g = v;
        return REG_DOMAIN_SCALER;
    }
    case REG_RGB_B: {
        uint16_t v = (value > 100) ? 100 : value;
        if (scaler_rgb_b == v)
            return REG_DOMAIN_NONE;
        scaler_rgb_b = v;
        return REG_DOMAIN_SCALER;
    }

    /*
     * Scale-up FIR filter. p1/p2 are stored PER FAMILY, so a param write updates
     * only the currently-active family's slot and switching family just re-points
     * at that family's stored tuning (no clobbering). Each write regenerates the
     * 128-byte table and pushes it via LOAD_FIR (REG_DOMAIN_FILTER).
     */
    case REG_FILTER_FAMILY: {
        uint16_t v = (value >= FIR_FAM_COUNT) ? FIR_FAM_MITCHELL : value;
        if (scaler_filter_family == v)
            return REG_DOMAIN_NONE;
        scaler_filter_family = v;
        return REG_DOMAIN_FILTER;
    }
    case REG_FILTER_P1: {
        uint8_t fam = (uint8_t)scaler_filter_family;
        uint8_t mx = fir_p1_max(fam);
        uint16_t v = (value > mx) ? mx : value;
        if (scaler_filter_p1[fam] == v)
            return REG_DOMAIN_NONE;
        scaler_filter_p1[fam] = v;
        return REG_DOMAIN_FILTER;
    }
    case REG_FILTER_P2: {
        uint8_t fam = (uint8_t)scaler_filter_family;
        uint8_t mx = fir_p2_max(fam);
        uint16_t v = (value > mx) ? mx : value;
        if (scaler_filter_p2[fam] == v)
            return REG_DOMAIN_NONE;
        scaler_filter_p2[fam] = v;
        return REG_DOMAIN_FILTER;
    }

    default:
        return REG_DOMAIN_NONE;
    }
}
