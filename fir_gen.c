#include "fir_gen.h"
#include <math.h>

/*
 * Hardware tap layout, reverse-engineered from the RTD's built-in tSU_COEF
 * tables (which display correctly):
 *   - 4 taps stored in this neighbour-offset order: {+2, +1, 0, -1}
 *   - 16 phases cover only HALF a pixel (f = phase/32); the scaler mirrors the
 *     upper half. Generating over a full pixel (phase/16) mis-samples every
 *     phase and drops thin columns at non-native resolutions.
 */
static const signed char OFFS[4] = { 2, 1, 0, -1 };
#define FIR_PHASE_DEN 32.0f

/* ---- per-family step index -> parameter value (0.05 steps, matches the UI) -- */
static float p1_value(uint8_t fam, uint8_t idx)
{
    switch (fam) {
    case FIR_FAM_MITCHELL: return 0.00f + idx * 0.05f;   /* B     0.00..1.00 */
    case FIR_FAM_KEYS:     return -1.00f + idx * 0.05f;  /* a    -1.00..0.00 */
    case FIR_FAM_GAUSS:    return 0.30f + idx * 0.05f;   /* sigma 0.30..1.50 */
    case FIR_FAM_LANCZOS:  return 0.0f;                  /* no parameter (order=2) */
    default:               return 0.50f;
    }
}
static float p2_value(uint8_t fam, uint8_t idx)         /* only Mitchell uses C */
{
    (void)fam;
    return 0.00f + idx * 0.05f;                          /* C 0.00..1.00 */
}

uint8_t fir_p1_max(uint8_t fam)
{
    switch (fam) {
    case FIR_FAM_MITCHELL: return 20;
    case FIR_FAM_KEYS:     return 20;
    case FIR_FAM_GAUSS:    return 24;
    case FIR_FAM_LANCZOS:  return 0;    /* parameterless */
    default:               return 20;
    }
}
uint8_t fir_p2_max(uint8_t fam)
{
    return (fam == FIR_FAM_MITCHELL) ? 20 : 0;
}

/* ---- kernels --------------------------------------------------------------- */
static float k_mitchell(float x, float B, float C)
{
    x = fabsf(x);
    if (x < 1.0f) return ((12-9*B-6*C)*x*x*x + (-18+12*B+6*C)*x*x + (6-2*B)) / 6.0f;
    if (x < 2.0f) return ((-B-6*C)*x*x*x + (6*B+30*C)*x*x + (-12*B-48*C)*x + (8*B+24*C)) / 6.0f;
    return 0.0f;
}
static float k_keys(float x, float a)
{
    x = fabsf(x);
    if (x < 1.0f) return (a+2)*x*x*x - (a+3)*x*x + 1.0f;
    if (x < 2.0f) return a*x*x*x - 5*a*x*x + 8*a*x - 4*a;
    return 0.0f;
}
static float k_gauss(float x, float s)
{
    return expf(-(x*x) / (2.0f * s * s));
}

/* Lanczos-2 (windowed sinc, order n=2): sharp reference kernel, rings the most */
static float k_lanczos(float x)
{
    float px, pxn;

    if (x == 0.0f) return 1.0f;
    if (x <= -2.0f || x >= 2.0f) return 0.0f;
    px = 3.14159265358979f * x;
    pxn = px / 2.0f;
    return 2.0f * sinf(px) * sinf(pxn) / (px * px);
}

static float kern(uint8_t fam, float pv1, float pv2, float x)
{
    switch (fam) {
    case FIR_FAM_MITCHELL: return k_mitchell(x, pv1, pv2);
    case FIR_FAM_KEYS:     return k_keys(x, pv1);
    case FIR_FAM_GAUSS:    return k_gauss(x, pv1);
    case FIR_FAM_LANCZOS:  return k_lanczos(x);
    default:               return k_mitchell(x, 0.40f, 0.55f);
    }
}

/* ---- table build ----------------------------------------------------------- */
/*
 * Output layout is tap-major, 12-bit signed little-endian: the coefficient for
 * tap group t (0..3) at phase ph (0..15) lives at byte offset (t*16 + ph)*2.
 * Each phase's four taps sum to exactly 1024. We fill out[] a phase at a time
 * (no intermediate table) to keep the MSP stack footprint small.
 */
void fir_generate(uint8_t family, uint8_t p1, uint8_t p2, uint8_t out[128])
{
    float pv1 = p1_value(family, p1);
    float pv2 = p2_value(family, p2);
    uint8_t ph, t;

    for (ph = 0; ph < 16; ph++) {
        float w[4], s = 0.0f;
        int q[4], sum = 0, maxi = 0;
        float f = ph / FIR_PHASE_DEN;

        for (t = 0; t < 4; t++) {
            w[t] = kern(family, pv1, pv2, (float)OFFS[t] - f);
            s += w[t];
        }
        if (s == 0.0f) s = 1.0f;
        for (t = 0; t < 4; t++) {
            float v = w[t] / s * 1024.0f;
            q[t] = (int)(v + (v >= 0.0f ? 0.5f : -0.5f));   /* round to nearest */
            sum += q[t];
            if (q[t] > q[maxi]) maxi = t;
        }
        q[maxi] += 1024 - sum;                              /* force phase sum = 1024 */

        for (t = 0; t < 4; t++) {
            uint16_t u = (uint16_t)q[t] & 0x0FFF;
            uint8_t off = (uint8_t)((t * 16 + ph) * 2);
            out[off]     = (uint8_t)(u & 0xFF);
            out[off + 1] = (uint8_t)((u >> 8) & 0xFF);
        }
    }
}
