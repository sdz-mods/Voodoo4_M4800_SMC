#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>

uint8_t monitor_is_due(void);
void monitor_run(void);
void monitor_update_host_response(void);

#endif
