#ifndef THERMAL_H
#define THERMAL_H

#include <stdint.h>

typedef struct {
    uint8_t external_temp_c;
    int32_t internal_temp_c;
    uint8_t fan_duty;
} thermal_sample_t;

void thermal_read(thermal_sample_t *sample);

#endif
