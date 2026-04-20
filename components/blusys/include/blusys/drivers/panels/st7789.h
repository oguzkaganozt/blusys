/**
 * @file st7789.h
 * @brief ST7789 panel hardware defaults.
 *
 * Provides panel-specific constants and a default ::blusys_lcd_config_t
 * pre-filled for a 240×320 ST7789 SPI panel in landscape orientation
 * (320 wide × 240 tall, achieved via `swap_xy` + `mirror_x`).
 *
 * Pin fields are left at `-1` (unspecified) — callers must set them for their
 * board.
 *
 * Hardware notes:
 *   - Some modules need `invert_color = true` for correct contrast.
 *   - Set `x_offset` / `y_offset` if the panel shows a border (bare modules).
 *   - Lower `pclk_hz` if you see tearing or bus errors on long wires.
 */

#ifndef BLUSYS_DRIVERS_PANELS_ST7789_H
#define BLUSYS_DRIVERS_PANELS_ST7789_H

#include "blusys/drivers/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Native panel width in pixels (portrait). */
#define BLUSYS_ST7789_NATIVE_WIDTH  240u
/** @brief Native panel height in pixels (portrait). */
#define BLUSYS_ST7789_NATIVE_HEIGHT 320u

/** @brief Logical width after landscape rotation (`swap_xy` + `mirror_x`). */
#define BLUSYS_ST7789_WIDTH  320u
/** @brief Logical height after landscape rotation. */
#define BLUSYS_ST7789_HEIGHT 240u

/** @brief Typical SPI pixel clock for ST7789 modules. */
#define BLUSYS_ST7789_PCLK_HZ 40000000u

/**
 * @brief Return a ::blusys_lcd_config_t pre-filled for a landscape ST7789 panel.
 *
 * Pin fields are `-1`; the caller must fill `sclk_pin`, `mosi_pin`, `cs_pin`,
 * `dc_pin`, and optionally `rst_pin` / `bl_pin` before passing the struct to
 * `blusys_lcd_open()`.
 */
static inline blusys_lcd_config_t blusys_st7789_default_config(void)
{
    blusys_lcd_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.driver         = BLUSYS_LCD_DRIVER_ST7789;
    cfg.width          = BLUSYS_ST7789_WIDTH;
    cfg.height         = BLUSYS_ST7789_HEIGHT;
    cfg.bits_per_pixel = 16;
    cfg.bgr_order      = false;
    cfg.swap_xy        = true;
    cfg.mirror_x       = true;
    cfg.mirror_y       = false;
    cfg.invert_color   = false;

    cfg.spi.pclk_hz  = BLUSYS_ST7789_PCLK_HZ;
    cfg.spi.x_offset = 0;
    cfg.spi.y_offset = 0;
    cfg.spi.sclk_pin = -1;
    cfg.spi.mosi_pin = -1;
    cfg.spi.cs_pin   = -1;
    cfg.spi.dc_pin   = -1;
    cfg.spi.rst_pin  = -1;
    cfg.spi.bl_pin   = -1;
    return cfg;
}

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_DRIVERS_PANELS_ST7789_H */
