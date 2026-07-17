#ifndef HOST_PROTOCOL_H
#define HOST_PROTOCOL_H

#include <stdint.h>

#define HOST_TX_BYTES 5
#define HOST_CMD_PREFIX 0x7f
#define HOST_CMD_SCALER_FULLSCREEN 0x10
#define HOST_CMD_SCALER_DOS_ASPECT 0x11

typedef struct {
    uint8_t backlight;
    uint8_t settings;
} host_request_t;

void host_protocol_init(void);
void host_protocol_poll_timeout(void);
void host_protocol_timer_tick(void);
uint8_t host_protocol_is_idle(void);
uint8_t host_protocol_take_request(host_request_t *request);
void host_protocol_set_response(const uint8_t response[HOST_TX_BYTES]);

#endif
