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

### SK6812 RGBW

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

**Wrong pin** — check that the chosen GPIO is a valid output pin on your target and not shared with flash, PSRAM, or other peripherals.

**Full brightness (255, 255, 255)** — white at full brightness draws about 60 mA per LED. Use values of 32 or lower for bench testing without a dedicated power supply.

**Forgetting `refresh`** — `set_pixel` only modifies the internal buffer. The strip will not update until `refresh` is called.

**Powering the strip from the board's 3.3 V rail** — addressable strips require 5 V. Use a level shifter or a 5 V-tolerant GPIO, and power the strip from a dedicated 5 V supply for anything more than a few LEDs.

**Using `set_pixel_rgbw` on a 3-byte strip** — returns `BLUSYS_ERR_NOT_SUPPORTED`. Only SK6812 RGBW strips have a white channel.

**Wrong type** — if colours appear scrambled (e.g. red and green swapped), check that the `type` field matches your actual LED chip. WS2812B and SK6812 use GRB; WS2811 and APA106 use RGB.

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Supported Chips

| Chip | Bytes/pixel | Colour Order | Notes |
|------|-------------|--------------|-------|
| WS2812B | 3 | GRB | Default. Most common addressable LED. |
| SK6812 RGB | 3 | GRB | Improved WS2812B alternative. |
| SK6812 RGBW | 4 | GRBW | Adds a dedicated white channel; use `set_pixel_rgbw`. |
| WS2811 | 3 | RGB | Often used in 12 V pixel modules. |
| APA106 | 3 | RGB | Through-hole addressable LED. |

All chips use the same single-wire protocol — only the timing and colour order differ. The driver selects the correct parameters from the `type` field in the config.

## Thread Safety

- `set_pixel` and `set_pixel_rgbw` write only to the internal pixel buffer and do not acquire a lock; if two tasks share a handle and write different pixels, they must coordinate externally before calling `refresh`
- `refresh` and `clear` hold the internal mutex for the entire transmit duration; a concurrent `refresh` from another task blocks until the current one finishes
- do not call `close` concurrently with `refresh` or `clear` on the same handle

## ISR Notes

No ISR-safe calls are defined for the LED strip module.

## Limitations

- no async transmit; `refresh` and `clear` block the calling task
- one RMT channel per handle; maximum simultaneous strips is bounded by available RMT channels on the target
- RGBW functions return `BLUSYS_ERR_NOT_SUPPORTED` on 3-byte strip types

## Example App

See `examples/validation/hal_io_lab` (LED strip scenario).
