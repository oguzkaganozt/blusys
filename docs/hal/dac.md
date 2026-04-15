# DAC

On-chip digital-to-analog converter: write an 8-bit voltage level to a DAC-capable GPIO.

## Quick Example

```c
#include <stdint.h>

#include "blusys/dac.h"

void app_main(void)
{
    blusys_dac_t *dac;

    if (blusys_dac_open(25, &dac) != BLUSYS_OK) {
        return;
    }

    blusys_dac_write(dac, 128u);
    blusys_dac_close(dac);
}
```

## Common Mistakes

- trying to use DAC on `ESP32-C3` or `ESP32-S3`
- selecting a GPIO that is not DAC-capable
- expecting audio streaming, waveform generation, or calibrated voltage units from the first DAC release

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | no |
| ESP32-S3 | no |

On ESP32-C3 and ESP32-S3, all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_DAC)` to check at runtime.

## Types

### `blusys_dac_t`

```c
typedef struct blusys_dac blusys_dac_t;
```

Opaque handle returned by `blusys_dac_open()`.

## Functions

### `blusys_dac_open`

```c
blusys_err_t blusys_dac_open(int pin, blusys_dac_t **out_dac);
```

Opens one DAC channel. Valid pins on ESP32: GPIO25 (channel 1), GPIO26 (channel 2).

**Parameters:**
- `pin` — DAC-capable GPIO number
- `out_dac` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins, `BLUSYS_ERR_BUSY` if the channel is already open, `BLUSYS_ERR_NOT_SUPPORTED` on unsupported targets.

---

### `blusys_dac_close`

```c
blusys_err_t blusys_dac_close(blusys_dac_t *dac);
```

Releases the DAC channel and frees the handle.

---

### `blusys_dac_write`

```c
blusys_err_t blusys_dac_write(blusys_dac_t *dac, uint8_t value);
```

Sets the DAC output to `value`. The output voltage is approximately `value / 255 × VDD`.

**Parameters:**
- `dac` — handle
- `value` — 8-bit output value (0 = 0V, 255 = VDD)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if called after `close` has started.

## Lifecycle

1. `blusys_dac_open()` — claim channel
2. `blusys_dac_write()` — set output voltage
3. `blusys_dac_close()` — release channel

## Thread Safety

- concurrent calls on the same handle are serialized internally
- do not call `blusys_dac_close()` concurrently with other calls on the same handle

## Limitations

- valid on GPIO25 and GPIO26 (ESP32 only)
- each DAC channel can be opened only once at a time
- only direct oneshot voltage output is supported; cosine generation and continuous DMA output are not exposed

## Example App

See `examples/validation/hal_io_lab` (DAC scenario).
