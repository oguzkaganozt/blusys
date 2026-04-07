# Drive an SPI LCD Display

## Problem Statement

You want to drive an SPI-connected LCD panel (such as the ST7789 or ST7735R) to display graphics or a solid color fill from an ESP32.

## Prerequisites

- an ESP32, ESP32-C3, or ESP32-S3 board
- an SPI LCD panel (e.g. ST7789 240x320 or ST7735R 128x160) wired to the board: SCLK, MOSI, CS, DC, and optionally RST and backlight

## Minimal Example

```c
#include "blusys/blusys_all.h"

static uint16_t row_buf[240];

void app_main(void)
{
    blusys_lcd_t *lcd;

    blusys_lcd_config_t config = {
        .driver         = BLUSYS_LCD_DRIVER_ST7789,
        .width          = 240,
        .height         = 320,
        .bits_per_pixel = 16,
        .bgr_order      = false,
        .spi = {
            .bus      = 0,
            .sclk_pin = 12,
            .mosi_pin = 11,
            .cs_pin   = 10,
            .dc_pin   = 13,
            .rst_pin  = -1,
            .bl_pin   = -1,
            .pclk_hz  = 20000000,
        },
    };

    blusys_lcd_open(&config, &lcd);

    /* fill red (RGB565) */
    for (int i = 0; i < 240; i++) {
        row_buf[i] = 0xF800;
    }
    for (int row = 0; row < 320; row++) {
        blusys_lcd_draw_bitmap(lcd, 0, row, 240, row + 1, row_buf);
    }

    blusys_lcd_close(lcd);
}
```

## APIs Used

- `blusys_lcd_open()` — creates the SPI bus, panel IO, and panel driver; resets and enables the display
- `blusys_lcd_draw_bitmap()` — sends pixel data for a rectangular region; blocking
- `blusys_lcd_close()` — turns the display off and releases all hardware resources

## Expected Runtime Behavior

- The display initializes and shows a solid red fill
- Each `draw_bitmap` call blocks until the SPI transfer completes
- After `close`, the SPI bus is freed and the display is off

## ST7735R (128x160) Example

For a 128x160 ST7735R TFT, use `BLUSYS_LCD_DRIVER_ST7735` and set the GRAM offsets. Most 128x160 modules need `x_offset = 2, y_offset = 1`:

```c
#include "blusys/blusys_all.h"

static uint16_t row_buf[128];

void app_main(void)
{
    blusys_lcd_t *lcd;

    blusys_lcd_config_t config = {
        .driver         = BLUSYS_LCD_DRIVER_ST7735,
        .width          = 128,
        .height         = 160,
        .bits_per_pixel = 16,
        .bgr_order      = false,
        .spi = {
            .bus      = 0,
            .sclk_pin = 12,
            .mosi_pin = 11,
            .cs_pin   = 10,
            .dc_pin   = 13,
            .rst_pin  = -1,
            .bl_pin   = -1,
            .pclk_hz  = 16000000,
            .x_offset = 2,
            .y_offset = 1,
        },
    };

    blusys_lcd_open(&config, &lcd);

    /* fill green (RGB565) */
    for (int i = 0; i < 128; i++) {
        row_buf[i] = 0x07E0;
    }
    for (int row = 0; row < 160; row++) {
        blusys_lcd_draw_bitmap(lcd, 0, row, 128, row + 1, row_buf);
    }

    blusys_lcd_close(lcd);
}
```

Color inversion is applied automatically for the ST7735 driver during `blusys_lcd_open()`.

## Common Mistakes

- **Wrong DC pin** — the Data/Command pin must be a valid output GPIO and correctly wired; swapping it with another signal produces a blank or garbled screen
- **BGR vs RGB** — if colors appear swapped (red shows as blue), set `bgr_order = true`
- **SPI bus conflict** — `blusys_lcd_open()` takes exclusive ownership of the SPI bus; do not call `blusys_spi_open()` on the same bus index
- **Missing reset wiring** — some panels require the RST line to be connected; if the display stays blank, wire RST and set `rst_pin` to the correct GPIO

## Example App

See `examples/lcd_basic/` (ST7789) and `examples/lcd_st7735_basic/` (ST7735R 128x160) for runnable examples with per-target configurable pins.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.

## API Reference

For full type definitions and function signatures, see [LCD API Reference](../modules/lcd.md).
