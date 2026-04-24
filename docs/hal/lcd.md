# LCD

Display panel driver for SPI/I2C panels via `esp_lcd`, plus a QEMU RGB driver for simulation.

## At a glance

- one handle per opened panel
- SPI and I2C transports are supported
- QEMU RGB ignores transport pins

## Quick example

```c
blusys_lcd_t *lcd;
blusys_lcd_config_t config = {
    .driver = BLUSYS_LCD_DRIVER_ST7789,
    .width = 240,
    .height = 320,
    .bits_per_pixel = 16,
};

blusys_lcd_open(&config, &lcd);
blusys_lcd_close(lcd);
```

## Common mistakes

- wrong DC pin
- RGB/BGR mismatch
- SPI or I2C bus conflicts
- missing reset wiring on some panels

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- all functions are thread-safe via an internal mutex
- draw calls block while the transfer is active

## Limitations

- the LCD module owns the underlying bus
- coordinates must be within panel dimensions
- full-refresh SPI is still experimental

## Example apps

- `examples/reference/display/`
- `examples/validation/peripheral_lab/` (`SSD1306`)
