#include <msp430.h>
#include <stdint.h>

#include "firmware_config.h"
#include "host_protocol.h"

#define HOST_RX_BYTES 2

static volatile uint8_t protocol_idle = 1;
static volatile uint8_t host_clock_timeout;
static volatile uint8_t rx_bit_count;
static volatile uint8_t rx_byte_count;
static volatile uint8_t rx_data[HOST_RX_BYTES];
static volatile uint8_t rx_packet_ready;
static volatile uint8_t host_read_active;
static volatile uint8_t tx_data[HOST_TX_BYTES];
static volatile uint8_t tx_bit_count;
static volatile uint8_t tx_byte_count;

void host_protocol_init(void)
{
    P4IES &= ~BIT1;
    P4IFG &= ~BIT1;
    P4IE |= BIT1;
}

void host_protocol_poll_timeout(void)
{
    if (host_clock_timeout != HOST_PACKET_TIMEOUT_TICKS)
        return;

    P4IE &= ~BIT1;
    rx_bit_count = 0;
    rx_byte_count = 0;
    host_clock_timeout = 0;
    host_read_active = 0;
    protocol_idle = 1;
    tx_bit_count = 0;
    tx_byte_count = 0;
    rx_data[0] = 0;
    rx_data[1] = 0;
    P4IFG &= ~BIT1;
    P4IE |= BIT1;
}

void host_protocol_timer_tick(void)
{
    if (host_clock_timeout < HOST_TIMEOUT_COUNTER_LIMIT)
        host_clock_timeout++;
}

uint8_t host_protocol_is_idle(void)
{
    return protocol_idle;
}

uint8_t host_protocol_take_request(host_request_t *request)
{
    if (!rx_packet_ready)
        return 0;

    P4IE &= ~BIT1;
    request->backlight = rx_data[0];
    request->settings = rx_data[1];
    rx_data[0] = 0;
    rx_data[1] = 0;
    rx_packet_ready = 0;
    P4IFG &= ~BIT1;
    P4IE |= BIT1;
    return 1;
}

void host_protocol_set_response(const uint8_t response[HOST_TX_BYTES])
{
    uint16_t status;
    uint8_t i;

    status = __get_SR_register();
    __disable_interrupt();
    for (i = 0; i < HOST_TX_BYTES; i++)
        tx_data[i] = response[i];
    if (status & GIE)
        __enable_interrupt();
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = PORT4_VECTOR
__interrupt void Port4_ISR(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(PORT4_VECTOR))) Port4_ISR(void)
#else
#error Compiler not supported!
#endif
{
    uint8_t tx_bit;

    if ((P4IFG & BIT1) == 0)
        return;

    P4IFG &= ~BIT1;
    host_clock_timeout = 0;
    protocol_idle = 0;

    if (!host_read_active) {
        rx_data[rx_byte_count] =
            (rx_data[rx_byte_count] << 1) | ((P4IN & BIT0) != 0);
        rx_bit_count++;

        if (rx_bit_count == 1 && rx_data[rx_byte_count] == 1) {
            rx_data[rx_byte_count] = 0;
            host_read_active = 1;
            rx_bit_count = 0;
            rx_byte_count = 0;
            return;
        }

        if (rx_bit_count == 8) {
            rx_bit_count = 0;
            rx_byte_count++;
            if (rx_byte_count == HOST_RX_BYTES) {
                rx_byte_count = 0;
                rx_packet_ready = 1;
                protocol_idle = 1;
            }
        }
        return;
    }

    tx_bit = tx_data[tx_byte_count] >> (7 - tx_bit_count);
    if (tx_bit & 1)
        P3OUT |= BIT7;
    else
        P3OUT &= ~BIT7;

    tx_bit_count++;
    if (tx_bit_count == 8) {
        tx_bit_count = 0;
        tx_byte_count++;
        if (tx_byte_count == HOST_TX_BYTES) {
            tx_byte_count = 0;
            host_read_active = 0;
            protocol_idle = 1;
        }
    }
}
