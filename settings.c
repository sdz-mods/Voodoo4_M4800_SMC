#include <stdint.h>

#include "settings.h"

#define HOST_BACKLIGHT_MASK 0x7f
#define HOST_VCORE_MASK 0x70
#define HOST_VCORE_SHIFT 4
#define HOST_FB_SIZE_MASK 0x08
#define HOST_FB_SIZE_SHIFT 3
#define HOST_BLANK_FIX_MASK 0x04
#define HOST_BLANK_FIX_SHIFT 2

/*
 * Keep this compatibility slot first. Existing cards store the remaining
 * settings at the following fixed FRAM addresses.
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

void settings_sanitize(void)
{
    if (backlight_percent > BACKLIGHT_MAX_PERCENT)
        backlight_percent = BACKLIGHT_MAX_PERCENT;
    if (vcore_setting > VCORE_MAX_SETTING)
        vcore_setting = VCORE_DEFAULT_SETTING;
    if (fbsize_setting > 1)
        fbsize_setting = FB_SIZE_DEFAULT_SETTING;
    vsa_blank_fix_en = !!vsa_blank_fix_en;
}

void settings_decode_host(uint8_t backlight, uint8_t settings)
{
    backlight_percent = backlight & HOST_BACKLIGHT_MASK;
    vcore_setting =
        (settings & HOST_VCORE_MASK) >> HOST_VCORE_SHIFT;
    fbsize_setting =
        (settings & HOST_FB_SIZE_MASK) >> HOST_FB_SIZE_SHIFT;
    vsa_blank_fix_en =
        (settings & HOST_BLANK_FIX_MASK) >> HOST_BLANK_FIX_SHIFT;
    settings_sanitize();
}

uint8_t settings_encode_host(void)
{
    return (vcore_setting << HOST_VCORE_SHIFT) |
        (fbsize_setting << HOST_FB_SIZE_SHIFT) |
        (vsa_blank_fix_en << HOST_BLANK_FIX_SHIFT);
}
