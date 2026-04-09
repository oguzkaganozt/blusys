# Drive a 7-Segment Display

## Problem Statement

You want to display numbers or hex values on a 7-segment display. The display may be driven directly by GPIO pins, through a 74HC595 shift register (to reduce pin usage), or via a MAX7219 SPI controller (for multi-digit hardware multiplexing and brightness control).

## Prerequisites

Choose one of the three wiring options below:

**Direct GPIO** — one output GPIO per segment (a–g, dp) plus one common pin per digit.

**74HC595** — three GPIO pins (DATA/CLK/LATCH) to the shift register's SER/SRCLK/RCLK inputs, plus one common pin per digit. Ideal for sourced single-digit displays where conserving GPIO pins matters.

**MAX7219** — three GPIO pins (MOSI/CLK/CS) to the MAX7219. No separate common pins — the chip handles multiplexing internally. A 10 µF capacitor between VCC and GND near the chip is recommended.

## Minimal Example

=== "MAX7219"

    ```c
    #include "blusys/drivers/display/seven_seg.h"

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

=== "74HC595"

    ```c
    #include "blusys/drivers/display/seven_seg.h"

    blusys_seven_seg_t *display;

    blusys_seven_seg_config_t cfg = {
        .driver      = BLUSYS_SEVEN_SEG_DRIVER_74HC595,
        .digit_count = 1,
        .hc595 = {
            .data_pin    = 13,
            .clk_pin     = 14,
            .latch_pin   = 15,
            .common_pins = { 16, -1, -1, -1, -1, -1, -1, -1 },
            .common_anode = false,
        },
    };

    blusys_seven_seg_open(&cfg, &display);
    blusys_seven_seg_write_int(display, 7, false);
    /* ... */
    blusys_seven_seg_close(display);
    ```

=== "Direct GPIO"

    ```c
    #include "blusys/drivers/display/seven_seg.h"

    blusys_seven_seg_t *display;

    blusys_seven_seg_config_t cfg = {
        .driver      = BLUSYS_SEVEN_SEG_DRIVER_GPIO,
        .digit_count = 1,
        .gpio = {
            .pin_a = 2,  .pin_b = 4,  .pin_c = 5,  .pin_d = 18,
            .pin_e = 19, .pin_f = 21, .pin_g = 22, .pin_dp = -1,
            .common_pins  = { 23, -1, -1, -1, -1, -1, -1, -1 },
            .common_anode = false,
        },
    };

    blusys_seven_seg_open(&cfg, &display);
    blusys_seven_seg_write_int(display, 5, false);
    /* ... */
    blusys_seven_seg_close(display);
    ```

## APIs Used

- `blusys_seven_seg_open()` — configures pins, starts the multiplexing timer (GPIO/74HC595) or initialises the MAX7219 over SPI
- `blusys_seven_seg_write_int()` — renders a signed decimal integer, right-aligned
- `blusys_seven_seg_write_hex()` — renders an unsigned hex value with leading zeros
- `blusys_seven_seg_write_raw()` — writes pre-encoded segment bitmasks directly
- `blusys_seven_seg_set_dp()` — controls decimal point segments independently
- `blusys_seven_seg_set_brightness()` — sets hardware brightness on MAX7219 (level 0–15)
- `blusys_seven_seg_clear()` — blanks all digits
- `blusys_seven_seg_close()` — stops the timer, resets GPIO, frees the handle

## Expected Runtime Behavior

- **GPIO / 74HC595:** digits are time-multiplexed at ~500 Hz (2 ms/digit). With 4 digits the effective refresh rate per digit is ~125 Hz — flicker-free at normal viewing distances.
- **MAX7219:** multiplexing is hardware-driven. The chip scans all active digits continuously with no CPU involvement.
- `write_int` returns `BLUSYS_ERR_INVALID_ARG` if the value does not fit in the configured number of digits (e.g., 9999 is the maximum for a 4-digit decimal display).

## Common Mistakes

- **Mismatched common anode/cathode setting.** If all segments light when they should be off (or nothing lights at all), check the `common_anode` field. Common-anode displays need active-low segment signals; common-cathode need active-high.
- **Not wiring digit select (common) pins.** For GPIO and 74HC595 drivers, a common pin must be configured for every digit you want to use. Set unused `common_pins[]` slots to -1.
- **MAX7219 digit count mismatch.** Set `digit_count` to the actual number of digits in your module. The MAX7219 scan limit is programmed from this value; unused digit registers must still be blanked (the driver does this on open).
- **Calling `set_brightness` on GPIO/74HC595.** These drivers do not support hardware brightness and return `BLUSYS_ERR_NOT_SUPPORTED`. Control brightness externally (e.g., current-limiting resistors).
- **SPI2_HOST already in use.** The MAX7219 driver initialises SPI2_HOST. If another peripheral already owns that bus, `blusys_seven_seg_open()` will return an error. Use a different SPI peripheral or driver.

## Example App

See `examples/seven_seg_basic/` for a runnable example covering all three driver types via menuconfig selection.

## API Reference

For full type definitions and function signatures, see [7-Segment API Reference](../modules/seven_seg.md).
