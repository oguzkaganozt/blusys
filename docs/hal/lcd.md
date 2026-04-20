# LCD

SPI and I2C display driver supporting ST7789, ST7735, SSD1306, NT35510, ILI9341, and ILI9488 panels via ESP-IDF's `esp_lcd`. A QEMU virtual-framebuffer driver (`BLUSYS_LCD_DRIVER_QEMU_RGB`) is available for QEMU-only builds.

## Quick Example

```c
#include "blusys/blusys.h"

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

Use the `spi` union member for ST7789, ST7735, NT35510, ILI9341, and ILI9488. Use the `i2c` union member for SSD1306. The QEMU_RGB driver is selected only in QEMU builds and ignores transport pins.

For common 128x160 ST7735 modules, portrait mode is often `swap_xy = true`, `mirror_x = true`.

## Common Mistakes

- **wrong DC pin** — the Data/Command pin must be a valid output GPIO and correctly wired; swapping it with another signal produces a blank or garbled screen
- **BGR vs RGB** — if colours appear swapped (red shows as blue), set `bgr_order = true`
- **SPI bus conflict** — `blusys_lcd_open()` takes exclusive ownership of the SPI bus; do not call `blusys_spi_open()` on the same bus index
- **I2C bus conflict** — `blusys_lcd_open()` takes exclusive ownership of the I2C bus; do not call `blusys_i2c_open()` on the same port
- **wrong I2C address** — SSD1306 modules use either `0x3C` or `0x3D`; if the screen stays blank, try the other address
- **missing reset wiring** — some panels require the RST line; if the display stays blank, wire RST and set `rst_pin` to the correct GPIO

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- all functions are thread-safe via an internal mutex
- `blusys_lcd_draw_bitmap()` holds the lock for the full duration of the pixel transfer (tens of milliseconds for large regions); other callers on the same handle block during this time

## ISR Notes

No ISR-safe calls are defined for the LCD module.

## Limitations

- the LCD module takes exclusive ownership of the SPI or I2C bus — do not use the same bus index with `blusys_spi_open()` or `blusys_i2c_open()`
- backlight control defaults to GPIO on/off; opt in to PWM dimming by setting `bl_ledc_enabled = true` and picking a free LEDC timer/channel
- coordinates passed to `blusys_lcd_draw_bitmap()` must be within the panel dimensions; no automatic clipping is performed
- on ST7735 panels over SPI, the driver issues **one display row per RAMWR** command (column/row address window is re-asserted per row) to avoid diagonal shear observed with multi-row pixel bursts — see `docs/internals/architecture.md#display-notes-st7735-on-spi`

## Example App

See `examples/reference/display/` (ST7789 / ST7735 scenarios) and `examples/validation/peripheral_lab/` (`periph_lcd_ssd1306` scenario for SSD1306 128x32 I2C OLED).
