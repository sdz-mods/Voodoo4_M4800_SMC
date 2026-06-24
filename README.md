# Voodoo4 M4800 LVDS/eDP SMC Firmware

Firmware for the MSP430FR2476 system management controller used on the
Voodoo4 M4800 MXM cards. It manages the card power sequence, thermal
monitoring, fan and panel backlight control, persistent card settings, and
the host control interface.

## Supported Hardware

| Card | Current Revision |
| --- | --- |
| V4 M4800 LVDS | A01 |
| V4 M4800 eDP | A00 |

**MCU:** MSP430FR2476TRHBR  
**CCS target device:** MSP430FR2476

The same firmware runs on both LVDS and eDP card variants. On LVDS cards,
the eDP bridge initialization writes have no adverse effect.

## Persistent Settings

Settings are stored in MSP430 FRAM and sanitized at startup and after each
host write. Their definitions and host packet encoding live in `settings.c`.

| Setting | Default | Values |
| --- | --- | --- |
| `backlight_percent` | 100 | 0-100% |
| `vcore_setting` | 1 | 0-6 |
| `fbsize_setting` | 0 | 0: 32 MB, 1: 64 MB |
| `vsa_blank_fix_en` | 0 | 0: disabled, 1: enabled |

### VSA-100 Core Voltage

| `vcore_setting` | Voltage |
| --- | --- |
| 0 | 2.5 V |
| 1 | 2.6 V |
| 2 | 2.7 V |
| 3 | 2.8 V |
| 4 | 2.9 V |
| 5 | 3.0 V |
| 6 | 3.1 V |

Board-tunable timing, sensor, timeout, and safety constants live in
`firmware_config.h`.

## Host Protocol

The host interface is a custom clocked three-wire serial protocol, carried
over GPIO pins on the XIO2001 PCIe-to-PCI bridge. It is SPI-like, but uses a
project-specific packet format and does not implement the SPI protocol.

| Signal | MSP430 pin |
| --- | --- |
| XIO clock input | `P4.1` |
| Host-to-card data | `P4.0` |
| Card-to-host data | `P3.7` |

Packet state, timeout handling, and the `P4.1` interrupt handler live in
`host_protocol.c`.

### Read Packet

Read transactions return five bytes:

| Byte | Value |
| --- | --- |
| 0 | Panel backlight percentage |
| 1 | Encoded voltage, framebuffer, and blank-fix settings |
| 2 | External GPU temperature |
| 3 | MSP430 internal temperature |
| 4 | Fan duty percentage |

### Write Packet

Write transactions contain two bytes:

| Byte | Value |
| --- | --- |
| 0 | Panel backlight percentage |
| 1 | Encoded voltage, framebuffer, and blank-fix settings |

Settings byte layout:

| Bits | Value |
| --- | --- |
| 7 | Reserved |
| 6:4 | `vcore_setting` |
| 3 | `fbsize_setting` |
| 2 | `vsa_blank_fix_en` |
| 1:0 | Reserved |

## Card Control

Framebuffer straps, VSA blank-fix output, and VSA core-voltage programming
are implemented in `card_control.c`.

### Framebuffer Straps

| Framebuffer | `P2.1` / D3 | `P3.4` / D2 |
| --- | --- | --- |
| 32 MB | High | Low |
| 64 MB | Low | High |

Framebuffer straps are applied at MCU power-up before the VSA-100 is powered,
and reapplied while external PCI reset is asserted.

The VSA blank-fix setting is applied at power-up, during external PCI reset,
and immediately after a host write.

## Display and I2C

`soft_I2C.c` implements software I2C using:

| Signal | MSP430 pin |
| --- | --- |
| SCL | `P3.6` |
| SDA | `P3.2` |

The LVDS-to-eDP bridge initialization sequence lives in `display.c`. It
configures dual-channel LVDS input, two-lane HBR eDP output, lane mapping,
transmitter amplitude and termination, and bridge reset state.

The MSP430 drives panel backlight PWM on `P2.0`. Optional gating from the
scaler backlight signal on `P4.2` is controlled by
`ENABLE_SCALER_BACKLIGHT_GATE`. It is currently disabled because the scaler
firmware does not behave as expected.

## Thermal and Power Management

The external card temperature sensor is selected with `TMP103_VARIANT`.
This board defaults to `TMP103_VARIANT_B`:

| Variant | 7-bit I2C address |
| --- | --- |
| TMP103A | `0x70` |
| TMP103B | `0x71` |
| TMP103C | `0x72` |

Temperature sampling and the fan curve live in `thermal.c`. Power-good
monitoring and over-temperature shutdown decisions live in `monitor.c`.

## GPIO Reference

| Pin | Function |
| --- | --- |
| `P1.0` | VCCINT power-good input |
| `P1.1` | VCCINT enable |
| `P1.2` | 2.5 V power-good input |
| `P1.3` | 2.5 V enable |
| `P1.4` | 3.3 V power-good input |
| `P1.5` | 3.3 V enable |
| `P1.6` | 1.8 V enable |
| `P1.7` | 1.8 V power-good input |
| `P2.0` | Panel backlight enable/PWM |
| `P2.1` | Framebuffer strap D3 |
| `P2.2` | MXM power-good output |
| `P2.3` | VSA blank-fix enable |
| `P2.4` | Scaler reset |
| `P2.5` | Scaler DDC1 SDA / UART TX input |
| `P2.6` | Scaler DDC1 SCL / UART RX input |
| `P2.7` | PCI reset input |
| `P3.0` | MXM power-enable input |
| `P3.1` | Fan PWM |
| `P3.2` | Software-I2C SDA |
| `P3.3` | MXM over-temperature output |
| `P3.4` | Framebuffer strap D2 / HD6 |
| `P3.5` | LCD enable |
| `P3.6` | Software-I2C SCL |
| `P3.7` | Host data output |
| `P4.0` | Host data input |
| `P4.1` | Host clock input |
| `P4.2` | Scaler backlight enable/PWM input |

## Source Layout

| File | Responsibility |
| --- | --- |
| `board_init.c` | GPIO directions, pull resistors, and initial output state |
| `card_control.c` | Framebuffer straps, blank fix, and VSA core voltage |
| `display.c` | LVDS-to-eDP bridge initialization |
| `hardware_init.c` | Clocks, timers, FRAM wait state, ADC, and temperature reference |
| `host_protocol.c` | Host packet state, timeout handling, and clock ISR |
| `monitor.c` | Power-good checks, thermal monitoring, and fault handling |
| `power.c` | Power-rail sequencing and latch-off |
| `pwm.c` | Fan and panel backlight software PWM |
| `settings.c` | Persistent settings and host packet encoding |
| `soft_I2C.c` | Software I2C implementation |
| `thermal.c` | Temperature acquisition and fan curve |

## Licensing

The `driverlib/` directory contains Texas Instruments MSP430 Driver Library
source code. It is distributed separately under the BSD 3-Clause license;
see `driverlib/LICENSE`.

Texas Instruments retains copyright over DriverLib. The project-specific
firmware source outside `driverlib/` is not covered by the DriverLib license.
