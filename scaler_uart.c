#include <msp430.h>
#include <stdint.h>

#include "scaler_uart.h"
#include "settings.h"

/*
 * Baud for SMCLK = 16 MHz, 19200 8N1, oversampling (UCOS16=1):
 *   UCBRW=52, UCBRF=1, UCBRS=0x49  ->  ~19200 Bd (well within tolerance).
 * (TI baud-rate table, 16 MHz BRCLK.)
 */
#define SCU_UCBRW   52
#define SCU_UCBRF   1
#define SCU_UCBRS   0x49

static volatile scaler_status_t g_status;

/* GET_STATUS liveness tracking */
static volatile uint8_t g_reply_seen;
static uint8_t g_miss;

/*
 * DOS-aspect boot override. While active, the scaler is forced to stretch DOS
 * modes (dos43 = 0) regardless of the stored scaler_dos43 setting. Active by
 * default from cold boot / PCI reset so boot-time low resolutions fill the
 * panel; SeaBIOS clears it at OS handoff so the stored dos43 (e.g. 4:3) applies.
 */
static uint8_t g_dos_override = 1;
static uint8_t g_dos43_sent = 0xFF;  /* last effective dos43 pushed (unknown) */

/* RX frame parser state */
static volatile uint8_t rx_state;   /* 0 sync,1 cmd,2 len,3 payload,4 cksum */
static volatile uint8_t rx_cmd, rx_len, rx_idx, rx_ck;
static volatile uint8_t rx_payload[20];

void scaler_uart_init(void)
{
    /*
     * Route P2.5 (UCA1RXD) and P2.6 (UCA1TXD) to eUSCI_A1.
     * NOTE: this uses the PRIMARY module function (SEL0=1, SEL1=0). If the
     * link is dead on hardware, the FR2476 pin table may place UCA1 on a
     * secondary/tertiary function here - adjust the SEL bits accordingly.
     */
    P2SEL0 |= BIT5 | BIT6;
    P2SEL1 &= ~(BIT5 | BIT6);

    UCA1CTLW0 = UCSWRST;                 /* hold in reset while configuring */
    UCA1CTLW0 |= UCSSEL__SMCLK;          /* clock from SMCLK (16 MHz) */
    UCA1BRW = SCU_UCBRW;
    UCA1MCTLW = ((uint16_t)SCU_UCBRS << 8) | (SCU_UCBRF << 4) | UCOS16;
    UCA1CTLW0 &= ~UCSWRST;               /* release for operation */

    UCA1IE |= UCRXIE;                    /* RX interrupt for reply parsing */
}

/* ---- TX (blocking per byte; the P4 host ISR still runs) ------------------ */

static void scu_tx_byte(uint8_t b)
{
    while (!(UCA1IFG & UCTXIFG))
        ;
    UCA1TXBUF = b;
}

static void scu_send_frame(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t i;
    uint8_t ck = cmd ^ len;

    scu_tx_byte(SCU_SYNC);
    scu_tx_byte(cmd);
    scu_tx_byte(len);
    for (i = 0; i < len; i++) {
        scu_tx_byte(payload[i]);
        ck ^= payload[i];
    }
    scu_tx_byte(ck);
}

static void scu_send_dos43_raw(uint8_t v)
{
    uint8_t p = v ? 1 : 0;
    scu_send_frame(SCU_CMD_SET_DOS43, &p, 1);
}

/*
 * Push the effective dos43 (0 while the boot override is active, otherwise the
 * stored setting). Skips a redundant resend so we never needlessly re-lock the
 * scaler mode (which blanks the screen briefly).
 */
static void scu_apply_dos43(void)
{
    uint8_t eff = g_dos_override ? 0 : (scaler_dos43 ? 1 : 0);

    if (eff == g_dos43_sent)
        return;
    g_dos43_sent = eff;
    scu_send_dos43_raw(eff);
}

void scaler_uart_set_dos_override(uint8_t active)
{
    g_dos_override = active ? 1 : 0;
    scu_apply_dos43();
}

void scaler_uart_send_sharpness(uint8_t v)
{
    scu_send_frame(SCU_CMD_SET_SHARPNESS, &v, 1);
}

void scaler_uart_send_contrast(uint8_t v)
{
    scu_send_frame(SCU_CMD_SET_CONTRAST, &v, 1);
}

void scaler_uart_send_peaking(uint8_t v)
{
    scu_send_frame(SCU_CMD_SET_PEAKING, &v, 1);
}

void scaler_uart_send_rgbgain(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t p[3];
    p[0] = r;
    p[1] = g;
    p[2] = b;
    scu_send_frame(SCU_CMD_SET_RGBGAIN, p, 3);
}

void scaler_uart_send_register(uint8_t index)
{
    switch (index) {
    case SCU_REG_DOS43:
        scu_apply_dos43();
        break;
    case SCU_REG_SHARPNESS:
        scaler_uart_send_sharpness((uint8_t)scaler_sharpness);
        break;
    case SCU_REG_CONTRAST:
        scaler_uart_send_contrast((uint8_t)scaler_contrast);
        break;
    case SCU_REG_PEAKING:
        scaler_uart_send_peaking((uint8_t)scaler_peaking);
        break;
    case SCU_REG_RGB_R:
    case SCU_REG_RGB_G:
    case SCU_REG_RGB_B:
        /* the scaler takes all three channels at once */
        scaler_uart_send_rgbgain((uint8_t)scaler_rgb_r,
                                 (uint8_t)scaler_rgb_g,
                                 (uint8_t)scaler_rgb_b);
        break;
    default:
        break;
    }
}

void scaler_uart_push_all(void)
{
    scu_apply_dos43();
    scaler_uart_send_sharpness((uint8_t)scaler_sharpness);
    scaler_uart_send_contrast((uint8_t)scaler_contrast);
    scaler_uart_send_peaking((uint8_t)scaler_peaking);
    scaler_uart_send_rgbgain((uint8_t)scaler_rgb_r,
                             (uint8_t)scaler_rgb_g,
                             (uint8_t)scaler_rgb_b);
}

void scaler_uart_request_status(void)
{
    /* age the link: if the previous request went unanswered, count a miss */
    if (!g_reply_seen) {
        if (g_status.link_ok && ++g_miss >= 3)
            g_status.link_ok = 0;
    } else {
        g_miss = 0;
    }
    g_reply_seen = 0;

    scu_send_frame(SCU_CMD_GET_STATUS, 0, 0);
}

const scaler_status_t *scaler_uart_status(void)
{
    return (const scaler_status_t *)&g_status;
}

/* ---- RX (interrupt-driven frame parser) --------------------------------- */

static void scu_handle_frame(void)
{
    if (rx_cmd == SCU_RSP_STATUS && rx_len >= 15) {
        g_status.rev0 = rx_payload[0];
        g_status.rev1 = rx_payload[1];
        g_status.rev2 = rx_payload[2];
        g_status.lock = rx_payload[3];
        g_status.in_width  = (uint16_t)rx_payload[4] | ((uint16_t)rx_payload[5] << 8);
        g_status.in_height = (uint16_t)rx_payload[6] | ((uint16_t)rx_payload[7] << 8);
        g_status.link_ok = 1;
        g_reply_seen = 1;
        g_miss = 0;
    } else if (rx_cmd == SCU_RSP_ACK) {
        /* an ACK also proves the link is alive */
        g_reply_seen = 1;
    }
}

static void scu_rx_byte(uint8_t b)
{
    switch (rx_state) {
    case 0:
        if (b == SCU_SYNC)
            rx_state = 1;
        break;
    case 1:
        rx_cmd = b;
        rx_ck = b;
        rx_state = 2;
        break;
    case 2:
        rx_len = b;
        rx_ck ^= b;
        rx_idx = 0;
        if (rx_len > sizeof(rx_payload))
            rx_state = 0;
        else
            rx_state = rx_len ? 3 : 4;
        break;
    case 3:
        rx_payload[rx_idx++] = b;
        rx_ck ^= b;
        if (rx_idx >= rx_len)
            rx_state = 4;
        break;
    case 4:
        if (b == rx_ck)
            scu_handle_frame();
        rx_state = 0;
        break;
    default:
        rx_state = 0;
        break;
    }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = EUSCI_A1_VECTOR
__interrupt void eUSCI_A1_ISR(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(EUSCI_A1_VECTOR))) eUSCI_A1_ISR(void)
#else
#error Compiler not supported!
#endif
{
    switch (__even_in_range(UCA1IV, USCI_UART_UCTXCPTIFG)) {
    case USCI_UART_UCRXIFG:
        scu_rx_byte(UCA1RXBUF);   /* reading RXBUF clears the flag */
        break;
    default:
        break;
    }
}
