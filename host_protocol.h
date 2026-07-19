#ifndef HOST_PROTOCOL_H
#define HOST_PROTOCOL_H

#include <stdint.h>

/* Read block: fixed, versioned. See monitor_update_host_response().
 * 23 base bytes + per-family filter params (family + p1[4] + p2[4] = 9). */
#define HOST_TX_BYTES 29
#define HOST_PROTO_VERSION 2

/*
 * Reserved "command" writes. A host write of [reg, value] with reg == 0x7f is
 * not a register store but a side-band command (value = sub-command). Used by
 * SeaBIOS to toggle the DOS-aspect boot override at OS handoff.
 */
#define HOST_CMD_PREFIX             0x7f
#define HOST_CMD_SCALER_FULLSCREEN  0x10  /* force DOS stretch (boot default) */
#define HOST_CMD_SCALER_DOS_ASPECT  0x11  /* restore stored dos43 (e.g. 4:3)  */

typedef struct {
    uint8_t reg;    /* XIO register index (settings.h REG_*) */
    uint8_t value;  /* value to write */
} host_request_t;

void host_protocol_init(void);
void host_protocol_poll_timeout(void);
void host_protocol_timer_tick(void);
uint8_t host_protocol_is_idle(void);
uint8_t host_protocol_take_request(host_request_t *request);
void host_protocol_set_response(const uint8_t response[HOST_TX_BYTES]);

#endif
