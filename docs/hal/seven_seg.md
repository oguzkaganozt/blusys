# 7-Segment Display

Unified 7-segment display driver supporting direct GPIO, 74HC595 shift-register, and MAX7219 SPI controller backends with up to 8 digits.

## Quick Example

```c
#include "blusys/blusys.h"

blusys_seven_seg_t *display;

blusys_seven_seg_config_t cfg = {
    .driver      = BLUSYS_SEVEN_SEG_DRIVER_MAX7219,
    .digit_count = 4,
    .max7219 = {
        .mosi_pin = 23,
        .clk_pin  = 18,
        .cs_pin   = 5,
    },
};

blusys_seven_seg_open(&cfg, &display);
blusys_seven_seg_write_int(display, 1234, false);
/* ... */
blusys_seven_seg_close(display);
```

For 74HC595 and direct GPIO configs, substitute the matching union member (`hc595` or `gpio`); the open/write/close calls are identical.

## Common Mistakes

- **mismatched common anode/cathode setting** — if all segments light when they should be off (or nothing lights at all), check the `common_anode` field
- **not wiring digit-select (common) pins** — for GPIO and 74HC595 backends, a common pin must be configured for every digit you want to use; set unused `common_pins[]` slots to `-1`
- **MAX7219 digit-count mismatch** — set `digit_count` to the actual number of digits in your module; the MAX7219 scan limit is programmed from this value
- **calling `set_brightness` on GPIO/74HC595** — these backends do not support hardware brightness and return `BLUSYS_ERR_NOT_SUPPORTED`; control brightness externally (current-limiting resistors)
- **SPI2_HOST already in use** — the MAX7219 backend initialises SPI2_HOST; if another peripheral already owns that bus, `open()` fails

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Backend Overview

| Backend | Pins | Multiplexing | Brightness control |
|---------|------|--------------|--------------------|
| `BLUSYS_SEVEN_SEG_DRIVER_GPIO` | 8 seg + N common | Software (`esp_timer`, 2 ms/digit) | Not supported |
| `BLUSYS_SEVEN_SEG_DRIVER_74HC595` | 3 shift-reg + N common | Software (`esp_timer`, 2 ms/digit) | Not supported |
| `BLUSYS_SEVEN_SEG_DRIVER_MAX7219` | 3 SPI (MOSI/CLK/CS) | Hardware (internal) | 16 levels (0–15) |

## Segment Encoding

The internal buffer and `write_raw` use this bitmask convention:

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

## Thread Safety

- all write functions take the internal mutex before modifying the display buffer; concurrent calls are safe
- the GPIO/74HC595 multiplexing timer reads the display buffer without holding the lock — safe because segment-byte updates are atomic on Xtensa and RISC-V
- do not call `close` concurrently with any write function on the same handle

## ISR Notes

No ISR-safe calls are defined for the 7-segment module.

## Limitations

- the MAX7219 backend uses SPI2_HOST exclusively; SPI3_HOST is not exposed
- `set_brightness` is not supported on GPIO or 74HC595 — control brightness externally
- GPIO/74HC595 multiplexing runs in the `esp_timer` task at 2 ms per digit; at 8 digits the per-digit refresh rate is ~62 Hz
- negative number display requires at least one extra digit for the minus sign — a 4-digit display shows values from `-999` to `9999`

## Example App

See `examples/validation/hal_io_lab` (seven-segment scenario).
