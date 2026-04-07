# Enumerate 1-Wire Devices

## Problem Statement

You want to discover all 1-Wire devices on a bus and read their unique 64-bit ROM codes, which identify the device family and serial number. The most common scenario is working with DS18B20 temperature sensors.

## Prerequisites

- any supported board (ESP32, ESP32-C3, or ESP32-S3)
- one GPIO with a 4.7 kΩ resistor to 3.3 V (the internal pull-up is too weak)
- at least one 1-Wire device connected to the same pin

**Wiring:**

```
ESP32 GPIO ──┬─── 4.7 kΩ ─── 3.3 V
             │
             └─── DQ pin of each 1-Wire device (all on the same wire)
```

All device GND pins connect to the board GND. For externally-powered devices, the VDD pin connects to 3.3 V separately.

## Minimal Example

```c
#include "blusys/blusys.h"

blusys_one_wire_t *ow;
blusys_one_wire_rom_t rom;
blusys_one_wire_config_t cfg = { .parasite_power = false };

blusys_one_wire_open(4, &cfg, &ow);

/* Single device: read ROM directly */
if (blusys_one_wire_read_rom(ow, &rom) == BLUSYS_OK) {
    printf("family=0x%02X serial=%02X%02X%02X%02X%02X%02X\n",
           rom.bytes[0],
           rom.bytes[1], rom.bytes[2], rom.bytes[3],
           rom.bytes[4], rom.bytes[5], rom.bytes[6]);
}

blusys_one_wire_close(ow);
```

## Enumerate Multiple Devices

```c
bool last = false;
blusys_one_wire_search_reset(ow);

while (!last) {
    if (blusys_one_wire_search(ow, &rom, &last) != BLUSYS_OK) break;
    printf("family=0x%02X\n", rom.bytes[0]);
}
```

## APIs Used

- `blusys_one_wire_open()` — allocates an RMT TX+RX channel pair on the specified GPIO; open-drain mode is set automatically
- `blusys_one_wire_reset()` — sends the reset pulse and returns `BLUSYS_ERR_NOT_FOUND` if no device responds with a presence pulse
- `blusys_one_wire_read_rom()` — convenience wrapper: reset + READ ROM (0x33) + 8 bytes read + CRC check; only works with exactly one device on the bus
- `blusys_one_wire_search_reset()` — clears SEARCH ROM state before starting enumeration
- `blusys_one_wire_search()` — one call per device; returns `BLUSYS_ERR_NOT_FOUND` when all devices have been returned
- `blusys_one_wire_close()` — disables the RMT channels and frees the handle

## Expected Runtime Behavior

Each `blusys_one_wire_search()` call takes approximately 1–2 ms (64 bit positions × 3 bus operations each). A full enumeration of ten devices takes roughly 15–20 ms.

The console output should look like:

```
[1]  family=0x28  serial=A1:B2:C3:D4:E5:F6  crc=0x5A
[2]  family=0x28  serial=11:22:33:44:55:66  crc=0x7B
found 2 device(s)
```

Family code `0x28` identifies a DS18B20. Other common codes: `0x10` (DS18S20), `0x22` (DS1822), `0x26` (DS2438).

## Common Mistakes

- **No pull-up resistor**: the bus stays LOW or floats; `blusys_one_wire_reset()` returns `BLUSYS_ERR_NOT_FOUND`
- **Pull-up too weak**: the internal pull-up (~45 kΩ) causes marginal timing; always use an external 4.7 kΩ
- **Using READ ROM with multiple devices**: when two devices share the bus simultaneously, their responses collide; use SEARCH ROM instead
- **Forgetting `search_reset` before a new pass**: `blusys_one_wire_search()` starts where it last left off; call `blusys_one_wire_search_reset()` to restart from the beginning
- **Sending a function command after SKIP ROM with multiple devices**: SKIP ROM addresses every device at once — sending a convert command is fine, but reading data back from multiple devices simultaneously will corrupt the result

## Example App

See `examples/one_wire_basic/` for a complete runnable example.

Set the GPIO pin in menuconfig under **Blusys 1-Wire Basic Example → 1-Wire bus GPIO pin**, then build and flash:

```
blusys run examples/one_wire_basic
```

## API Reference

For full type definitions and function signatures, see [1-Wire API Reference](../modules/one_wire.md).
