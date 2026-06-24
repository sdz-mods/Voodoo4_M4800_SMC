#include <stdint.h>

#include "display.h"
#include "soft_I2C.h"

void display_bridge_init(void)
{
    typedef struct {
        uint8_t address;
        uint8_t reg;
        uint8_t value;
    } display_register_t;
    static const display_register_t registers[] = {
        { 0x70, 0x0f, 0x01 }, /* Disable reset. */
        { 0x70, 0x00, 0x00 }, /* Select dual-channel LVDS input. */
        { 0x70, 0x02, 0x07 }, /* Enable adaptive panel parameters. */
        { 0x70, 0x03, 0x04 }, /* Swap LVDS A/B; JEIDA, normal polarity. */
        { 0x70, 0x07, 0xc2 }, /* Enable 24-bpp, 2-lane HBR eDP and training. */
        { 0x70, 0x0b, 0x00 }, /* Disable built-in self-test. */
        { 0x75, 0x00, 0xb0 }, /* Release analog reset and enable HBR. */
        { 0x75, 0x84, 0x10 }, /* Map transmitter lanes 0 and 1 directly. */
        { 0x75, 0x85, 0x32 }, /* Map transmitter lanes 2 and 3 directly. */
        { 0x75, 0x0e, 0x08 }, /* Transmitter amplitude; driver used 0x06. */
        { 0x75, 0x0f, 0x05 }, /* Transmitter amplitude; driver used 0x06. */
        { 0x75, 0x11, 0x00 }, /* Transmitter termination; driver used 0x33. */
        { 0x75, 0x22, 0x04 }, /* RGB SDR timing, rising edge, SDR, power down, LVDS lock manual mode disable, LVDS A/B channel power down normal. */
        { 0x75, 0x00, 0xb1 }, /* Enable dual-channel LVDS input. */
        { 0x70, 0x0f, 0x00 }, /* Enable reset. */
    };
    uint8_t i;

    for (i = 0; i < sizeof(registers) / sizeof(registers[0]); i++)
        sI2C_write_reg(registers[i].address, registers[i].reg,
                       registers[i].value);
}
