# LED Strip

WS2812B addressable LED strip driver with single-wire GRB protocol via RMT at 10 MHz.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [Drive an LED Strip](../guides/led-strip-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Protocol

WS2812B uses a single-wire 800 kHz protocol with GRB byte order (24 bits per pixel, MSB first). The RMT peripheral is clocked at 10 MHz (100 ns/tick) to meet the ±150 ns timing tolerances. The API accepts colors as (r, g, b) and converts to GRB internally.

| Symbol | HIGH | LOW |
|--------|------|-----|
| Bit 0 | 400 ns (4 ticks) | 900 ns (9 ticks) |
| Bit 1 | 800 ns (8 ticks) | 500 ns (5 ticks) |
| Reset | — | 60 µs (600 ticks) |

## Types

### `blusys_led_strip_t`

```c
typedef struct blusys_led_strip blusys_led_strip_t;
```

Opaque handle returned by `blusys_led_strip_open()`.

### `blusys_led_strip_config_t`

```c
typedef struct {
    int      pin;        /* GPIO pin connected to DIN */
    uint32_t led_count;  /* number of LEDs in the strip */
} blusys_led_strip_config_t;
```

## Functions

### `blusys_led_strip_open`

```c
blusys_err_t blusys_led_strip_open(const blusys_led_strip_config_t *config,
                                    blusys_led_strip_t **out_strip);
```

Allocates a handle, opens an RMT TX channel at 10 MHz, and installs the WS2812B byte and reset encoders. Initializes the pixel buffer to all-off.

**Parameters:**
- `config` — pin and LED count; `led_count` must be greater than zero
- `out_strip` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pin or zero LED count, `BLUSYS_ERR_NO_MEM` on allocation failure.

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

Writes an (r, g, b) color into the internal pixel buffer. The strip does not update until `blusys_led_strip_refresh()` is called.

**Parameters:**
- `strip` — handle
- `index` — zero-based LED index; must be less than `led_count`
- `r`, `g`, `b` — color components (0–255 each)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `index` is out of range.

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

Sets all pixels to off (0, 0, 0) and calls `blusys_led_strip_refresh()`.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `strip` is NULL, `BLUSYS_ERR_TIMEOUT` if the transmit does not complete in time.

---

## Lifecycle

```
blusys_led_strip_open()
    │
    ├── blusys_led_strip_set_pixel()   ← paint pixels into buffer
    ├── blusys_led_strip_refresh()     ← transmit buffer to hardware
    │
    └── blusys_led_strip_close()
```

## Thread Safety

- `set_pixel` writes only to the internal pixel buffer and does not acquire a lock. If two tasks share a handle and write different pixels, they must coordinate externally before calling `refresh`.
- `refresh` and `clear` hold the internal mutex for the entire transmit duration. A concurrent `refresh` from another task blocks until the current one finishes.
- Do not call `close` concurrently with `refresh` or `clear` on the same handle.

## Limitations

- WS2812B protocol only (GRB, 800 kHz). SK6812 RGBW is not supported.
- No async transmit; `refresh` and `clear` block the calling task.
- One RMT channel per handle; maximum simultaneous strips is limited by available RMT channels on the target.

## Example App

See `examples/led_strip_basic/`.
