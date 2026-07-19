#ifndef FIR_GEN_H
#define FIR_GEN_H

#include <stdint.h>

/*
 * Scale-up FIR kernel generator for the RTD2662 scaler.
 *
 * Produces the exact 128-byte table (16 phases x 4 taps, tap-major, 12-bit
 * signed little-endian, each phase summing to 1024) that the scaler's LOAD_FIR
 * command expects. This is the same math as the laptop tuning console, so a
 * (family, p1, p2) chosen there yields an identical filter here.
 *
 * p1/p2 are STEP INDICES; the MSP maps them to the real parameter value per
 * family (0.05 steps, matching the tuning UI):
 *   Mitchell : p1 = B (0.00..1.00, 21 steps),  p2 = C (0.00..1.00, 21 steps)
 *   Keys     : p1 = a (-1.00..0.00, 21 steps), p2 unused
 *   Gaussian : p1 = sigma (0.30..1.50, 25 steps), p2 unused
 *   Lanczos  : order fixed at 2 (4-tap); p1/p2 unused
 */

enum {
    FIR_FAM_MITCHELL = 0,
    FIR_FAM_KEYS     = 1,
    FIR_FAM_GAUSS    = 2,
    FIR_FAM_LANCZOS  = 3,
    FIR_FAM_COUNT    = 4
};

/* Largest valid step index for each param of a family (0 if the param is unused). */
uint8_t fir_p1_max(uint8_t family);
uint8_t fir_p2_max(uint8_t family);

/* Build the 128-byte scale-up FIR table for (family, p1, p2) into out[]. */
void fir_generate(uint8_t family, uint8_t p1, uint8_t p2, uint8_t out[128]);

#endif /* FIR_GEN_H */
