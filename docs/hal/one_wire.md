# 1-Wire

Dallas/Maxim 1-Wire protocol over a single GPIO, implemented using the RMT peripheral for precise timing. Supports device enumeration (SEARCH ROM) and direct addressing (READ ROM, MATCH ROM, SKIP ROM). The most common use case is reading DS18B20 temperature sensors.

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

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

All three targets include the RMT peripheral. On unsupported platforms all functions return `BLUSYS_ERR_NOT_SUPPORTED`.

**Hardware requirement:** connect a 4.7 kΩ pull-up resistor from the data pin to 3.3 V. The ESP32 internal pull-up (~45 kΩ) is too weak for reliable 1-Wire communication.

## Types

### `blusys_one_wire_t`

```c
typedef struct blusys_one_wire blusys_one_wire_t;
```

Opaque handle returned by `blusys_one_wire_open()`.

### `blusys_one_wire_rom_t`

```c
typedef struct {
    uint8_t bytes[8]; /* [0]=family code, [1..6]=48-bit serial, [7]=CRC-8 */
} blusys_one_wire_rom_t;
```

64-bit device address as defined by the 1-Wire specification. `bytes[0]` identifies the device family (e.g. `0x28` for DS18B20). `bytes[7]` is a Dallas/Maxim CRC-8 over bytes 0–6.

### `blusys_one_wire_config_t`

```c
typedef struct {
    bool parasite_power;
} blusys_one_wire_config_t;
```

- `parasite_power` — set `true` if devices draw power from the data line rather than a separate VDD pin. Stored but no strong pull-up is applied in the current implementation; an external 4.7 kΩ pull-up provides the necessary idle-high voltage.

## Functions

### `blusys_one_wire_open`

```c
blusys_err_t blusys_one_wire_open(int pin,
                                   const blusys_one_wire_config_t *config,
                                   blusys_one_wire_t **out_ow);
```

Opens the 1-Wire bus on `pin`. Allocates and enables an RMT TX channel (open-drain, idle HIGH) and an RMT RX channel on the same GPIO.

**Parameters:**
- `pin` — output-capable GPIO connected to the bus data line
- `config` — bus configuration; must not be NULL
- `out_ow` — receives the handle on success

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`, `BLUSYS_ERR_NOT_SUPPORTED`, or a translated ESP-IDF error on RMT driver failure.

---

### `blusys_one_wire_close`

```c
blusys_err_t blusys_one_wire_close(blusys_one_wire_t *ow);
```

Disables and releases both RMT channels, then frees the handle. Do not call while another operation on the same handle is in progress.

---

### `blusys_one_wire_reset`

```c
blusys_err_t blusys_one_wire_reset(blusys_one_wire_t *ow);
```

Sends a 480 µs LOW reset pulse, then listens for a presence pulse (60–300 µs LOW) from any device.

**Returns:** `BLUSYS_OK` if at least one device responded, `BLUSYS_ERR_NOT_FOUND` if no presence pulse was detected.

---

### `blusys_one_wire_write_bit`

```c
blusys_err_t blusys_one_wire_write_bit(blusys_one_wire_t *ow, bool bit);
```

Writes one bit onto the bus (6 µs LOW + 64 µs HIGH for bit 1; 60 µs LOW + 10 µs HIGH for bit 0).

---

### `blusys_one_wire_read_bit`

```c
blusys_err_t blusys_one_wire_read_bit(blusys_one_wire_t *ow, bool *out_bit);
```

Reads one bit. Master drives LOW for 6 µs then releases; if the bus returns HIGH within 15 µs the bit is 1, otherwise 0.

---

### `blusys_one_wire_write_byte`

```c
blusys_err_t blusys_one_wire_write_byte(blusys_one_wire_t *ow, uint8_t byte);
```

Writes one byte, LSB first. All 8 bit slots are transmitted in a single RMT transfer.

---

### `blusys_one_wire_read_byte`

```c
blusys_err_t blusys_one_wire_read_byte(blusys_one_wire_t *ow, uint8_t *out_byte);
```

Reads one byte, LSB first. All 8 read-initiation slots are transmitted in a single RMT transfer and the bus response is captured in one RX operation.

---

### `blusys_one_wire_write`

```c
blusys_err_t blusys_one_wire_write(blusys_one_wire_t *ow, const uint8_t *data, size_t len);
```

Writes `len` bytes by calling `blusys_one_wire_write_byte()` for each byte.

---

### `blusys_one_wire_read`

```c
blusys_err_t blusys_one_wire_read(blusys_one_wire_t *ow, uint8_t *data, size_t len);
```

Reads `len` bytes by calling `blusys_one_wire_read_byte()` for each byte.

---

### `blusys_one_wire_read_rom`

```c
blusys_err_t blusys_one_wire_read_rom(blusys_one_wire_t *ow,
                                       blusys_one_wire_rom_t *out_rom);
```

Issues reset + `0x33` (READ ROM) + reads 8 bytes. Validates the CRC-8 before returning.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_NOT_FOUND` if no device, `BLUSYS_ERR_IO` on CRC mismatch. Only valid when exactly one device is on the bus.

---

### `blusys_one_wire_skip_rom`

```c
blusys_err_t blusys_one_wire_skip_rom(blusys_one_wire_t *ow);
```

Issues reset + `0xCC` (SKIP ROM). Addresses all devices simultaneously. Caller sends the function command next.

---

### `blusys_one_wire_match_rom`

```c
blusys_err_t blusys_one_wire_match_rom(blusys_one_wire_t *ow,
                                        const blusys_one_wire_rom_t *rom);
```

Issues reset + `0x55` (MATCH ROM) + 8-byte address. Addresses one specific device. Caller sends the function command next.

---

### `blusys_one_wire_search`

```c
blusys_err_t blusys_one_wire_search(blusys_one_wire_t *ow,
                                     blusys_one_wire_rom_t *out_rom,
                                     bool *out_last_device);
```

Enumerates one device per call using the SEARCH ROM algorithm (Maxim Application Note 187). Call `blusys_one_wire_search_reset()` before starting a new pass, then call `blusys_one_wire_search()` repeatedly until `*out_last_device` is `true`.

**Returns:** `BLUSYS_OK` with `out_rom` filled, `BLUSYS_ERR_NOT_FOUND` when all devices have been returned or the bus is empty, `BLUSYS_ERR_IO` on CRC failure.

---

### `blusys_one_wire_search_reset`

```c
blusys_err_t blusys_one_wire_search_reset(blusys_one_wire_t *ow);
```

Clears SEARCH ROM state so the next call to `blusys_one_wire_search()` starts from the beginning.

---

### `blusys_one_wire_crc8`

```c
uint8_t blusys_one_wire_crc8(const uint8_t *data, size_t len);
```

Computes Dallas/Maxim CRC-8 (polynomial 0x31, reflected as 0x8C) over `len` bytes. To verify a ROM code: `blusys_one_wire_crc8(rom.bytes, 7)` should equal `rom.bytes[7]`.

## Lifecycle

1. `blusys_one_wire_open()` — allocate handle, enable RMT TX+RX
2. `blusys_one_wire_reset()` — detect presence
3. Send ROM command (`read_rom`, `skip_rom`, `match_rom`, or `search`)
4. Send device function command + exchange data bytes
5. Repeat from step 2 for each transaction
6. `blusys_one_wire_close()` — release resources

## Thread Safety

- Individual operations (`reset`, `write_byte`, `read_byte`, etc.) are serialized internally via a mutex and a bus-busy guard
- `blusys_one_wire_search()` executes multiple primitive calls sequentially without holding the lock between them; call it from a single task at a time
- Do not call `blusys_one_wire_close()` while another operation is in progress

## Limitations

- **External pull-up required**: the internal pull-up (~45 kΩ) is too weak; a 4.7 kΩ resistor to 3.3 V is mandatory
- **Parasite power**: the idle-high state via the pull-up resistor is sufficient for most parasite-powered devices at room temperature; long cable runs or temperature conversions that require a strong pull-up are not supported in this version
- **One bus per handle**: a single handle owns one GPIO and cannot share an RMT channel with another driver

## Example App

See `examples/validation/one_wire_basic/`.
