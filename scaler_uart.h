#ifndef SCALER_UART_H
#define SCALER_UART_H

#include <stdint.h>

/*
 * UART link to the RTD2662 scaler (firmware rev A01+).
 * eUSCI_A1, 19200 8N1: TX = P2.6 -> scaler pin 58 (RXD),
 *                      RX = P2.5 <- scaler pin 59 (TXD).
 * Frame (both directions): [0xAA sync][cmd][len][payload..][cksum]
 *   cksum = cmd ^ len ^ payload[0] ^ ... ^ payload[len-1]
 * The MSP is the master; the scaler only replies when asked.
 */

/* command codes (must match the RTD firmware Uart.h) */
#define SCU_SYNC            0xAA
#define SCU_CMD_SET_DOS43       0x01
#define SCU_CMD_SET_SHARPNESS   0x02
#define SCU_CMD_SET_CONTRAST    0x03
#define SCU_CMD_SET_PEAKING     0x04
#define SCU_CMD_SET_RGBGAIN     0x05
#define SCU_CMD_LOAD_FIR        0x06   /* 128-byte scale-up FIR table */
#define SCU_CMD_GET_STATUS      0x10
#define SCU_RSP_ACK             0x81
#define SCU_RSP_STATUS          0x90

/* XIO read-block register indices that map to scaler settings */
#define SCU_REG_DOS43       0x10
#define SCU_REG_SHARPNESS   0x11
#define SCU_REG_CONTRAST    0x12
#define SCU_REG_PEAKING     0x13
#define SCU_REG_RGB_R       0x14
#define SCU_REG_RGB_G       0x15
#define SCU_REG_RGB_B       0x16

/* cached live status, refreshed by GET_STATUS replies (RX interrupt) */
typedef struct {
    uint8_t  link_ok;   /* scaler answered a recent GET_STATUS */
    uint8_t  lock;      /* a mode is locked and displayed */
    uint16_t in_width;  /* measured input width */
    uint16_t in_height; /* measured input height */
    uint8_t  rev0;      /* firmware revision chars, e.g. 'A' '0' '1' */
    uint8_t  rev1;
    uint8_t  rev2;
} scaler_status_t;

void scaler_uart_init(void);

/* send one setting to the scaler (blocking TX, interrupts stay enabled) */
void scaler_uart_send_sharpness(uint8_t v);
void scaler_uart_send_contrast(uint8_t v);
void scaler_uart_send_peaking(uint8_t v);
void scaler_uart_send_rgbgain(uint8_t r, uint8_t g, uint8_t b);

/*
 * Regenerate the scale-up FIR from the stored filter params (family/p1/p2) and
 * push it as a 128-byte LOAD_FIR frame. This is the "sharpness" the panel
 * actually reacts to; the legacy SET_SHARPNESS table select is not used.
 *
 * The push is ~67 ms (128 bytes @ 19200). To avoid blocking the host protocol
 * once per write during an Apply burst, filter register writes only flag it
 * dirty (scaler_uart_mark_filter_dirty); the main loop flushes it ONCE, from
 * scaler_uart_flush_filter(), after the host has gone quiet.
 */
void scaler_uart_apply_filter(void);
void scaler_uart_mark_filter_dirty(void);
uint8_t scaler_uart_filter_pending(void);
void scaler_uart_flush_filter(void);

/* send the scaler setting identified by an XIO register index (SCU_REG_*) */
void scaler_uart_send_register(uint8_t index);

/*
 * Enable/disable the DOS-aspect boot override. Active forces the scaler to
 * stretch DOS modes (dos43 = 0); inactive restores the stored dos43. Active by
 * default at boot; SeaBIOS clears it at OS handoff. Idempotent (deduped).
 */
void scaler_uart_set_dos_override(uint8_t active);

/* push every scaler setting from FRAM (call once the scaler is up) */
void scaler_uart_push_all(void);

/* fire a GET_STATUS; the reply lands asynchronously in the RX interrupt */
void scaler_uart_request_status(void);

const scaler_status_t *scaler_uart_status(void);

#endif
