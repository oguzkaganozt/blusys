# LED Strip

Addressable LED strip driver supporting WS2812B, SK6812 (RGB and RGBW), WS2811, and APA106 via RMT at 10 MHz.

## Quick Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_led_strip_t *strip;
    const blusys_led_strip_config_t cfg = {
        .pin       = 18,
        .led_count = 8,
        .type      = BLUSYS_LED_STRIP_WS2812B,
    };

    if (blusys_led_strip_open(&cfg, &strip) != BLUSYS_OK) {
        return;
    }

    blusys_led_strip_set_pixel(strip, 0, 0, 32, 0);  /* pixel 0: green */
    blusys_led_strip_set_pixel(strip, 1, 32, 0, 0);  /* pixel 1: red   */
    blusys_led_strip_refresh(strip, BLUSYS_TIMEOUT_FOREVER);

    blusys_led_strip_close(strip);
}
```

### SK6812 RGBW Example

```c
const blusys_led_strip_config_t cfg = {
    .pin       = 18,
    .led_count = 8,
    .type      = BLUSYS_LED_STRIP_SK6812_RGBW,
};

blusys_led_strip_open(&cfg, &strip);
blusys_led_strip_set_pixel_rgbw(strip, 0, 0, 0, 0, 64);  /* warm white */
blusys_led_strip_refresh(strip, BLUSYS_TIMEOUT_FOREVER);
```

## Common Mistakes

**Wrong pin** — check that the chosen GPIO is a valid output pin on your target and that it is not shared with flash, PSRAM, or other peripherals.

**Full brightness (255, 255, 255)** — white at full brightness draws approximately 60 mA per LED. Use values of 32 or lower for bench testing without a dedicated power supply.

**Forgetting `refresh`** — `set_pixel` only modifies the internal buffer. The strip will not update until `refresh` is called.

**Powering the strip from the board's 3.3 V rail** — addressable LED strips require 5 V. Use a level shifter or a 5 V-tolerant GPIO, and power the strip from a dedicated 5 V supply for anything more than a few LEDs.

**Using `set_pixel_rgbw` on a 3-byte strip** — returns `BLUSYS_ERR_NOT_SUPPORTED`. Only SK6812 RGBW strips have a white channel.

**Wrong type** — if colors appear scrambled (e.g., red and green swapped), check that the `type` field matches your actual LED chip. WS2812B and SK6812 use GRB order while WS2811 and APA106 use RGB order.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Supported Chips

| Chip | Bytes/pixel | Color Order | Notes |
|------|-------------|-------------|-------|
| WS2812B | 3 | GRB | Default. Most common addressable LED. |
| SK6812 RGB | 3 | GRB | Improved WS2812B alternative. |
| SK6812 RGBW | 4 | GRBW | Adds a dedicated white channel. Use `set_pixel_rgbw`. |
| WS2811 | 3 | RGB | Often used in 12 V pixel modules. |
| APA106 | 3 | RGB | Through-hole addressable LED. |

All chips use the same single-wire protocol — only the timing and color order differ. The driver selects the correct parameters from the `type` field in the config.

## Types

### `blusys_led_strip_type_t`

```c
typedef enum {
    BLUSYS_LED_STRIP_WS2812B = 0,   /* 3-byte GRB, default */
    BLUSYS_LED_STRIP_SK6812_RGB,     /* 3-byte GRB */
    BLUSYS_LED_STRIP_SK6812_RGBW,    /* 4-byte GRBW */
    BLUSYS_LED_STRIP_WS2811,         /* 3-byte RGB */
    BLUSYS_LED_STRIP_APA106,         /* 3-byte RGB */
} blusys_led_strip_type_t;
```

### `blusys_led_strip_t`

```c
typedef struct blusys_led_strip blusys_led_strip_t;
```

Opaque handle returned by `blusys_led_strip_open()`.

### `blusys_led_strip_config_t`

```c
typedef struct {
    int                     pin;        /* GPIO pin connected to DIN */
    uint32_t                led_count;  /* number of LEDs in the strip */
    blusys_led_strip_type_t type;       /* LED chip type (default: WS2812B) */
} blusys_led_strip_config_t;
```

## Functions

### `blusys_led_strip_open`

```c
blusys_err_t blusys_led_strip_open(const blusys_led_strip_config_t *config,
                                    blusys_led_strip_t **out_strip);
```

Allocates a handle, opens an RMT TX channel at 10 MHz, and installs the byte and reset encoders for the selected chip type. Initializes the pixel buffer to all-off.

**Parameters:**

- `config` — pin, LED count, and chip type; `led_count` must be greater than zero
- `out_strip` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pin, zero LED count, or unknown type, `BLUSYS_ERR_NO_MEM` on allocation failure.

---

### `blusys_led_strip_close`

```c
blusys_err_t blusys_led_strip_close(blusys_led_strip_t *strip);
```

Disables and deletes the RMT channel and encoders, then frees the handle and pixel buffer.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `strip` is NULL.

---

### `blusys_led_strip_set_pixel`

```c
blusys_err_t blusys_led_strip_set_pixel(blusys_led_strip_t *strip,
                                         uint32_t index,
                                         uint8_t r, uint8_t g, uint8_t b);
```

Writes an (r, g, b) color into the internal pixel buffer. The driver converts to the chip's native color order internally. The strip does not update until `blusys_led_strip_refresh()` is called.

For RGBW strips (SK6812 RGBW), this function sets the white channel to 0. Use `blusys_led_strip_set_pixel_rgbw()` to control the white channel.

**Parameters:**

- `strip` — handle
- `index` — zero-based LED index; must be less than `led_count`
- `r`, `g`, `b` — color components (0-255 each)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `index` is out of range.

---

### `blusys_led_strip_set_pixel_rgbw`

```c
blusys_err_t blusys_led_strip_set_pixel_rgbw(blusys_led_strip_t *strip,
                                              uint32_t index,
                                              uint8_t r, uint8_t g, uint8_t b,
                                              uint8_t w);
```

Writes an (r, g, b, w) color into the internal pixel buffer. Only valid for 4-byte RGBW strip types (SK6812 RGBW).

**Parameters:**

- `strip` — handle
- `index` — zero-based LED index; must be less than `led_count`
- `r`, `g`, `b`, `w` — color components (0-255 each)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `index` is out of range, `BLUSYS_ERR_NOT_SUPPORTED` if the strip type is not RGBW.

---

### `blusys_led_strip_refresh`

```c
blusys_err_t blusys_led_strip_refresh(blusys_led_strip_t *strip, int timeout_ms);
```

Transmits the current pixel buffer to the strip via RMT, followed by a reset pulse to latch the data. Blocks until both transmissions complete.

**Parameters:**

- `strip` — handle
- `timeout_ms` — milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` if the transmit does not complete in time.

---

### `blusys_led_strip_clear`

```c
blusys_err_t blusys_led_strip_clear(blusys_led_strip_t *strip, int timeout_ms);
```

Sets all pixels to off (0, 0, 0[, 0]) and calls `blusys_led_strip_refresh()`.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `strip` is NULL, `BLUSYS_ERR_TIMEOUT` if the transmit does not complete in time.

---

## Lifecycle

```
blusys_led_strip_open()
    |
    |-- blusys_led_strip_set_pixel()        <- paint RGB pixels
    |-- blusys_led_strip_set_pixel_rgbw()   <- paint RGBW pixels (SK6812 RGBW only)
    |-- blusys_led_strip_refresh()          <- transmit buffer to hardware
    |
    +-- blusys_led_strip_close()
```

## Thread Safety

- `set_pixel` and `set_pixel_rgbw` write only to the internal pixel buffer and do not acquire a lock. If two tasks share a handle and write different pixels, they must coordinate externally before calling `refresh`.
- `refresh` and `clear` hold the internal mutex for the entire transmit duration. A concurrent `refresh` from another task blocks until the current one finishes.
- Do not call `close` concurrently with `refresh` or `clear` on the same handle.

## Limitations

- No async transmit; `refresh` and `clear` block the calling task.
- One RMT channel per handle; maximum simultaneous strips is limited by available RMT channels on the target.
- RGBW functions return `BLUSYS_ERR_NOT_SUPPORTED` when used with 3-byte strip types.

## Example App

See `examples/validation/hal_io_lab` (LED strip scenario).
