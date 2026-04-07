# Drive an LED Strip

## Problem Statement

You want to drive a WS2812B (NeoPixel) LED strip from an ESP32 without writing low-level RMT timing code.

## Prerequisites

- a supported board (ESP32, ESP32-C3, or ESP32-S3)
- a WS2812B LED strip with its DIN connected to one board-safe output GPIO
- 5 V power for the strip (a separate supply is recommended for strips longer than 8 LEDs)

## Minimal Example

```c
#include "blusys/display/led_strip.h"

void app_main(void)
{
    blusys_led_strip_t *strip;
    const blusys_led_strip_config_t cfg = {
        .pin       = 18,
        .led_count = 8,
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

## APIs Used

- `blusys_led_strip_open()` — allocates a handle and opens an RMT channel at 10 MHz
- `blusys_led_strip_set_pixel()` — writes (r, g, b) into the internal GRB buffer; the strip does not change yet
- `blusys_led_strip_refresh()` — transmits the buffer and sends the reset pulse to latch data into the LEDs
- `blusys_led_strip_clear()` — sets all pixels to off and calls refresh; useful for startup
- `blusys_led_strip_close()` — disables the RMT channel and frees all resources

## Expected Runtime Behavior

- `set_pixel` returns immediately; the strip does not change until `refresh` is called
- `refresh` blocks until both the pixel data and the reset pulse have been transmitted
- colors are given as (r, g, b) in the API; the driver converts to the GRB on-wire order internally

## Common Mistakes

**Wrong pin** — check that the chosen GPIO is a valid output pin on your target and that it is not shared with flash, PSRAM, or other peripherals.

**Full brightness (255, 255, 255)** — white at full brightness draws approximately 60 mA per LED. Use values of 32 or lower for bench testing without a dedicated power supply.

**Forgetting `refresh`** — `set_pixel` only modifies the internal buffer. The strip will not update until `refresh` is called.

**Powering the strip from the board's 3.3 V rail** — WS2812B strips require 5 V. Use a level shifter or a 5 V-tolerant GPIO, and power the strip from a dedicated 5 V supply for anything more than a few LEDs.

## Example App

See `examples/led_strip_basic/`.

## API Reference

For full type definitions and function signatures, see [LED Strip API Reference](../modules/led_strip.md).
