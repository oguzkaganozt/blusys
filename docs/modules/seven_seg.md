# 7-Segment Display

Unified 7-segment display service supporting direct GPIO, 74HC595 shift-register, and MAX7219 SPI controller drivers with up to 8 digits.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [Drive a 7-Segment Display](../guides/seven-seg-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Driver Overview

| Driver | Pins | Multiplexing | Brightness control |
|--------|------|-------------|-------------------|
| `BLUSYS_SEVEN_SEG_DRIVER_GPIO` | 8 seg + N common | Software (esp_timer, 2 ms/digit) | Not supported |
| `BLUSYS_SEVEN_SEG_DRIVER_74HC595` | 3 shift-reg + N common | Software (esp_timer, 2 ms/digit) | Not supported |
| `BLUSYS_SEVEN_SEG_DRIVER_MAX7219` | 3 SPI (MOSI/CLK/CS) | Hardware (internal) | 16 levels (0–15) |

## Segment Encoding

The internal buffer and `write_raw` use the following bitmask convention:

| Bit | Segment |
|-----|---------|
| 0 | a — top |
| 1 | b — top-right |
| 2 | c — bottom-right |
| 3 | d — bottom |
| 4 | e — bottom-left |
| 5 | f — top-left |
| 6 | g — middle |
| 7 | dp — decimal point |

## Types

### `blusys_seven_seg_driver_t`

```c
typedef enum {
    BLUSYS_SEVEN_SEG_DRIVER_GPIO    = 0,
    BLUSYS_SEVEN_SEG_DRIVER_74HC595 = 1,
    BLUSYS_SEVEN_SEG_DRIVER_MAX7219 = 2,
} blusys_seven_seg_driver_t;
```

### `blusys_seven_seg_gpio_config_t`

```c
typedef struct {
    int  pin_a, pin_b, pin_c, pin_d, pin_e, pin_f, pin_g;
    int  pin_dp;                                   /* -1 = not wired */
    int  common_pins[BLUSYS_SEVEN_SEG_MAX_DIGITS]; /* -1 = unused slot */
    bool common_anode;
} blusys_seven_seg_gpio_config_t;
```

One output GPIO per segment. `common_pins[i]` selects digit `i`; unused slots must be `-1`.

### `blusys_seven_seg_595_config_t`

```c
typedef struct {
    int  data_pin;                                 /* SER */
    int  clk_pin;                                  /* SRCLK */
    int  latch_pin;                                /* RCLK */
    int  common_pins[BLUSYS_SEVEN_SEG_MAX_DIGITS]; /* -1 = unused slot */
    bool common_anode;
} blusys_seven_seg_595_config_t;
```

Segment data is shifted into the 74HC595 MSB-first on each multiplexing tick. `common_pins` work the same as in the GPIO config.

### `blusys_seven_seg_max7219_config_t`

```c
typedef struct {
    int mosi_pin;
    int clk_pin;
    int cs_pin;
} blusys_seven_seg_max7219_config_t;
```

Uses SPI2_HOST at up to 10 MHz. No separate common pins — the MAX7219 handles digit scanning internally.

### `blusys_seven_seg_config_t`

```c
typedef struct {
    blusys_seven_seg_driver_t driver;
    uint8_t                   digit_count; /* 1–8 */
    union {
        blusys_seven_seg_gpio_config_t    gpio;
        blusys_seven_seg_595_config_t     hc595;
        blusys_seven_seg_max7219_config_t max7219;
    };
} blusys_seven_seg_config_t;
```

### `blusys_seven_seg_t`

```c
typedef struct blusys_seven_seg blusys_seven_seg_t;
```

Opaque handle returned by `blusys_seven_seg_open()`.

## Functions

### `blusys_seven_seg_open`

```c
blusys_err_t blusys_seven_seg_open(const blusys_seven_seg_config_t *config,
                                   blusys_seven_seg_t **out_display);
```

Allocates and initialises the display handle. For GPIO and 74HC595 drivers, configures GPIO pins and starts a periodic esp_timer at 2 ms/digit for software multiplexing. For MAX7219, initialises SPI2_HOST, programs the scan limit and intensity registers, blanks all digits, and brings the chip out of shutdown mode.

**Parameters:**
- `config` — driver configuration; `digit_count` must be 1–8
- `out_display` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL or out-of-range parameters, `BLUSYS_ERR_NO_MEM` on allocation failure, `BLUSYS_ERR_IO` if SPI bus initialisation fails.

---

### `blusys_seven_seg_close`

```c
blusys_err_t blusys_seven_seg_close(blusys_seven_seg_t *display);
```

Stops and deletes the multiplexing timer (GPIO/74HC595), sends the MAX7219 shutdown command (MAX7219), resets GPIO pins, and frees the handle.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `display` is NULL.

---

### `blusys_seven_seg_write_int`

```c
blusys_err_t blusys_seven_seg_write_int(blusys_seven_seg_t *display,
                                        int value, bool leading_zeros);
```

Renders a signed integer right-aligned. If `leading_zeros` is true, unused high-order digits show `0`; otherwise they are blank. A minus sign uses the leftmost digit for negative values.

**Parameters:**
- `value` — value to display
- `leading_zeros` — pad with `0` instead of blank

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if the value does not fit in `digit_count` decimal digits.

---

### `blusys_seven_seg_write_hex`

```c
blusys_err_t blusys_seven_seg_write_hex(blusys_seven_seg_t *display, uint32_t value);
```

Renders a hex value (0–F digits) right-aligned with leading zeros.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if the value exceeds `16^digit_count - 1`.

---

### `blusys_seven_seg_write_raw`

```c
blusys_err_t blusys_seven_seg_write_raw(blusys_seven_seg_t *display,
                                        const uint8_t *segments, uint8_t count);
```

Copies `count` segment bitmasks into the display buffer starting at digit 0 (leftmost). Use the bit encoding table above to compose custom characters.

**Parameters:**
- `segments` — array of bitmasks, one per digit
- `count` — number of elements; must be ≤ `digit_count`

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL or out-of-range `count`.

---

### `blusys_seven_seg_set_dp`

```c
blusys_err_t blusys_seven_seg_set_dp(blusys_seven_seg_t *display, uint8_t digit_mask);
```

Sets the decimal point for each digit in `digit_mask` (bit N = digit N, leftmost is bit 0). Existing segment data is preserved; only the dp bit (bit 7) is modified. Pass `0x00` to clear all decimal points.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `display` is NULL.

---

### `blusys_seven_seg_set_brightness`

```c
blusys_err_t blusys_seven_seg_set_brightness(blusys_seven_seg_t *display, uint8_t level);
```

Sets the MAX7219 hardware intensity register. Level 0 is minimum brightness, 15 is maximum.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_NOT_SUPPORTED` for GPIO and 74HC595 drivers, `BLUSYS_ERR_INVALID_ARG` if `level > 15`.

---

### `blusys_seven_seg_clear`

```c
blusys_err_t blusys_seven_seg_clear(blusys_seven_seg_t *display);
```

Blanks all digits (sets all segment bitmasks to 0x00).

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `display` is NULL.

---

## Lifecycle

```
blusys_seven_seg_open()
    │
    ├── blusys_seven_seg_write_int()   ← display decimal numbers
    ├── blusys_seven_seg_write_hex()   ← display hex values
    ├── blusys_seven_seg_write_raw()   ← custom segment patterns
    ├── blusys_seven_seg_set_dp()      ← toggle decimal points
    ├── blusys_seven_seg_set_brightness() ← MAX7219 only
    ├── blusys_seven_seg_clear()       ← blank display
    │
    └── blusys_seven_seg_close()
```

## Thread Safety

- All write functions (`write_int`, `write_hex`, `write_raw`, `set_dp`, `set_brightness`, `clear`) take the internal mutex before modifying the display buffer. Concurrent calls from multiple tasks are safe.
- The GPIO/74HC595 multiplexing timer reads the display buffer without holding the lock. This is safe because segment bitmask updates are byte-wide and therefore atomic on Xtensa and RISC-V.
- Do not call `close` concurrently with any write function on the same handle.

## Limitations

- The MAX7219 driver uses SPI2_HOST exclusively. If another peripheral is already using SPI2_HOST, `open` will fail. SPI3_HOST (ESP32 only) is not exposed.
- `set_brightness` is not supported on GPIO or 74HC595 drivers. Control brightness externally through current-limiting resistors.
- GPIO/74HC595 multiplexing runs in the esp_timer task at 2 ms per digit. At 8 digits the per-digit refresh rate is ~62 Hz. Increasing `digit_count` beyond 8 is not supported.
- Negative number display requires at least one extra digit for the minus sign. A 4-digit display can show values from -999 to 9999.

## Example App

See `examples/seven_seg_basic/`.
