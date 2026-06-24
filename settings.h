#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

#define BACKLIGHT_MAX_PERCENT 100
#define VCORE_DEFAULT_SETTING 1
#define VCORE_MAX_SETTING 6
#define FB_SIZE_DEFAULT_SETTING 0

extern uint16_t backlight_percent;
extern uint16_t vcore_setting;
extern uint16_t fbsize_setting;
extern uint16_t vsa_blank_fix_en;

void settings_sanitize(void);
void settings_decode_host(uint8_t backlight, uint8_t settings);
uint8_t settings_encode_host(void);

#endif
