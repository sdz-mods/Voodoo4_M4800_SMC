#include <msp430.h>
#include <stdint.h>

#include "firmware_config.h"
#include "hardware_init.h"

/*
 * Timer0 ISR (~133 kHz) drives the 10-step software PWM. The FAN uses it every
 * ISR -> ~13 kHz fan PWM (it stalls at low PWM frequencies). The BACKLIGHT is
 * prescaled inside the ISR (see pwm.c) to ~0.4 kHz so its low-duty on-pulses
 * are long (~240 us) and immune to ISR jitter, which otherwise stretched the
 * one-ISR-period 10% pulse and made a dim panel flicker brighter.
 */
#define TIMER0_PWM_PERIOD_COUNTS 120
#define TIMER1_MONITOR_PERIOD_COUNTS 32768

static void software_trim_dco(void)
{
    uint16_t oldDcoTap = 0xffff;
    uint16_t newDcoTap = 0xffff;
    uint16_t newDcoDelta = 0xffff;
    uint16_t bestDcoDelta = 0xffff;
    uint16_t csCtl0Copy = 0;
    uint16_t csCtl1Copy = 0;
    uint16_t csCtl0Read = 0;
    uint16_t csCtl1Read = 0;
    uint16_t dcoFreqTrim = 3;
    uint8_t endLoop = 0;

    do {
        CSCTL0 = 0x100;
        do {
            CSCTL7 &= ~DCOFFG;
        } while (CSCTL7 & DCOFFG);

        __delay_cycles((uint16_t)3000 * MCLK_FREQ_MHZ);
        while ((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) &&
               ((CSCTL7 & DCOFFG) == 0))
            ;

        csCtl0Read = CSCTL0;
        csCtl1Read = CSCTL1;
        oldDcoTap = newDcoTap;
        newDcoTap = csCtl0Read & 0x01ff;
        dcoFreqTrim = (csCtl1Read & 0x0070) >> 4;

        if (newDcoTap < 256) {
            newDcoDelta = 256 - newDcoTap;
            if (oldDcoTap != 0xffff && oldDcoTap >= 256) {
                endLoop = 1;
            } else {
                dcoFreqTrim--;
                CSCTL1 =
                    (csCtl1Read & ~DCOFTRIM) | (dcoFreqTrim << 4);
            }
        } else {
            newDcoDelta = newDcoTap - 256;
            if (oldDcoTap < 256) {
                endLoop = 1;
            } else {
                dcoFreqTrim++;
                CSCTL1 =
                    (csCtl1Read & ~DCOFTRIM) | (dcoFreqTrim << 4);
            }
        }

        if (newDcoDelta < bestDcoDelta) {
            csCtl0Copy = csCtl0Read;
            csCtl1Copy = csCtl1Read;
            bestDcoDelta = newDcoDelta;
        }
    } while (endLoop == 0);

    CSCTL0 = csCtl0Copy;
    CSCTL1 = csCtl1Copy;
    while (CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1))
        ;
}

void hardware_init_clocks_timers_adc(void)
{
    /* One FRAM wait state is required before running MCLK above 8 MHz. */
    FRCTL0 = FRCTLPW | NWAITS_1;

    __bis_SR_register(SCG0);
    CSCTL3 |= SELREF__REFOCLK;
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_5;
    CSCTL2 = FLLD_0 + 487;
    __delay_cycles(10);
    __bic_SR_register(SCG0);
    software_trim_dco();
    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;

    /* Timer0 drives the 10-step software PWM at about 132 kHz. */
    TA0CCTL0 |= CCIE;
    TA0CCR0 = TIMER0_PWM_PERIOD_COUNTS;
    TA0CTL = TASSEL__SMCLK | MC__UP;

    /* Timer1 requests monitoring and advances protocol timeout once a second. */
    TA1CCTL0 |= CCIE;
    TA1CCR0 = TIMER1_MONITOR_PERIOD_COUNTS;
    TA1CTL = TASSEL__ACLK | MC__UP;

    ADCCTL0 |= ADCSHT_8 | ADCON;
    ADCCTL1 |= ADCSHP;
    ADCCTL2 &= ~ADCRES;
    ADCCTL2 |= ADCRES_2;
    ADCMCTL0 |= ADCSREF_1 | ADCINCH_12;
    PMMCTL0_H = PMMPW_H;
    PMMCTL2 |= INTREFEN | TSENSOREN | REFVSEL_0;
    __delay_cycles(400);
}
