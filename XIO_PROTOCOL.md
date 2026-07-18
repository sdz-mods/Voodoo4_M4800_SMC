# V4 M4800 XIO Host Protocol (version 2)

The host (BIOS setup, DOS tool, Windows app) talks to the MSP over the
bit-banged XIO link (P4.1 clock, P4.0 host->card data, P3.7 card->host data).
The first clocked bit selects direction: **1 = read, 0 = write**.

Version 2 adds the scaler settings (pushed to the RTD2662 over UART) to the
existing power/backlight settings. Detect version via read byte [0].

## Write transaction — 2 bytes: `[index, value]`

Each write sets exactly one register. `index` MUST be < 0x80 (its top bit is
the write-direction bit). Out-of-range values are clamped by the MSP.

| index | setting        | range           | domain  |
|-------|----------------|-----------------|---------|
| 0x00  | backlight %    | 0..100          | power   |
| 0x01  | vcore setting  | 0..6            | power   |
| 0x02  | fbsize         | 0..1            | power   |
| 0x03  | vsa_blank_fix  | 0..1            | power   |
| 0x10  | dos43 rule     | 0..1            | scaler  |
| 0x11  | sharpness      | 0..4            | scaler  |
| 0x12  | contrast       | 0..100          | scaler  |
| 0x13  | peaking        | 0..100 (0=off)  | scaler  |
| 0x14  | rgb gain R     | 0..100 (50=neu) | scaler  |
| 0x15  | rgb gain G     | 0..100 (50=neu) | scaler  |
| 0x16  | rgb gain B     | 0..100 (50=neu) | scaler  |

- power-domain writes apply immediately (backlight PWM, VSA vcore, blank-fix).
- scaler-domain writes update FRAM and send the matching UART command to the
  RTD. (Writing any of R/G/B sends all three channels together.)
- All settings persist in MSP FRAM.

### Side-band command: `index == 0x7f`

A write with `index == 0x7f` is not a register store but a command; `value` is
the sub-command. Used by SeaBIOS to control the **DOS-aspect boot override**.

| value | command          | effect                                          |
|-------|------------------|-------------------------------------------------|
| 0x10  | scaler fullscreen | force DOS stretch (scaler dos43 = 0)           |
| 0x11  | scaler dos aspect | restore the stored dos43 rule (e.g. 4:3)        |

The override is **active by default** from cold boot and re-asserted on every
PCI reset, so boot-time low resolutions fill the panel. SeaBIOS sends `0x7f
0x11` at OS handoff (`call_boot_entry`/`boot_cbfs`) so the stored dos43 setting
takes effect once the OS is running. The override forces the *effective* dos43
without changing the stored value; sends are deduped to avoid mode re-locks.

## Read transaction — fixed 20-byte block

Live data is at the front so a host can do a **short read of just the first
6 bytes** (proto + scaler status) for a fast status refresh, and the full
20-byte read only when it needs the settings too.

| byte | field            | notes                                   |
|------|------------------|-----------------------------------------|
| 0    | proto_version    | = 2                                     |
| 1    | scaler flags     | bit0 = link_ok, bit1 = input locked     |
| 2    | input width  LSB | measured by scaler (little-endian)      |
| 3    | input width  MSB |                                         |
| 4    | input height LSB |                                         |
| 5    | input height MSB | short "status" read = bytes 0-5         |
| 6    | ext temp (C)     | live                                    |
| 7    | int temp (C)     | live                                    |
| 8    | fan duty %       | live                                    |
| 9    | backlight %      |                                         |
| 10   | vcore setting    |                                         |
| 11   | fbsize           |                                         |
| 12   | vsa_blank_fix    |                                         |
| 13   | dos43            |                                         |
| 14   | sharpness        |                                         |
| 15   | contrast         |                                         |
| 16   | peaking          |                                         |
| 17   | rgb gain R       |                                         |
| 18   | rgb gain G       |                                         |
| 19   | rgb gain B       |                                         |

Bytes 1-5 come from the MSP polling the scaler's GET_STATUS once per second;
`link_ok` = 0 means the scaler has not answered recently. A short read stops
after byte 5, which leaves the MSP's read state mid-block until its ~2 s idle
timeout - the host must not start another transaction until then.

## Defaults (fresh FRAM)

backlight 100, vcore 1, fbsize 0, blank_fix 0,
dos43 1, sharpness 2, contrast 40, peaking 0, rgb 50/50/50.

## Upgrade note

Version 2 changes the write format from the old combined
`[backlight, settings]` to indexed `[index, value]`. Flash the MSP and update
the BIOS together; apps should check read byte [0] for the version.

## Underlying scaler UART

See the RTD firmware `UART_DEFAULTS.txt`. Link is eUSCI_A1, 19200 8N1,
P2.6=TX -> scaler pin 58, P2.5=RX <- scaler pin 59. Frame:
`[0xAA][cmd][len][payload..][xor-cksum]`. The MSP is the sole master.
