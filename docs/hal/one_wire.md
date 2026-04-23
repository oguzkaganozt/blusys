# 1-Wire

Dallas/Maxim 1-Wire protocol over a single GPIO, implemented using the RMT peripheral for precise timing. Supports device enumeration (SEARCH ROM) and direct addressing (READ ROM, MATCH ROM, SKIP ROM). The most common use case is reading DS18B20 temperature sensors.

**Hardware requirement:** connect a 4.7 kΩ pull-up resistor from the data pin to 3.3 V. The ESP32 internal pull-up (~45 kΩ) is too weak for reliable 1-Wire communication.

## Quick Example

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

## Common Mistakes

- **No pull-up resistor**: the bus stays LOW or floats; `blusys_one_wire_reset()` returns `BLUSYS_ERR_NOT_FOUND`
- **Pull-up too weak**: the internal pull-up (~45 kΩ) causes marginal timing; always use an external 4.7 kΩ
- **Using READ ROM with multiple devices**: when two devices share the bus simultaneously, their responses collide; use SEARCH ROM instead
- **Forgetting `search_reset` before a new pass**: `blusys_one_wire_search()` starts where it last left off; call `blusys_one_wire_search_reset()` to restart from the beginning
- **Sending a function command after SKIP ROM with multiple devices**: SKIP ROM addresses every device at once — sending a convert command is fine, but reading data back from multiple devices simultaneously will corrupt the result

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

All three targets include the RMT peripheral. On unsupported platforms all functions return `BLUSYS_ERR_NOT_SUPPORTED`.

## Thread Safety

- Individual operations (`reset`, `write_byte`, `read_byte`, etc.) are serialized internally via a mutex and a bus-busy guard
- `blusys_one_wire_search()` executes multiple primitive calls sequentially without holding the lock between them; call it from a single task at a time
- Do not call `blusys_one_wire_close()` while another operation is in progress

## Limitations

- **External pull-up required**: the internal pull-up (~45 kΩ) is too weak; a 4.7 kΩ resistor to 3.3 V is mandatory
- **Parasite power**: the idle-high state via the pull-up resistor is sufficient for most parasite-powered devices at room temperature; long cable runs or temperature conversions that require a strong pull-up are not supported in this version
- **One bus per handle**: a single handle owns one GPIO and cannot share an RMT channel with another driver

## Example App

See `examples/validation/hal_io_lab` (1-Wire scenario).
