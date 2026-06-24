#ifndef PWM_H
#define PWM_H

#include <stdint.h>

void pwm_set_backlight_percent(uint16_t percent);
void pwm_set_fan_duty(uint16_t duty);
uint16_t pwm_get_fan_duty(void);

#endif
