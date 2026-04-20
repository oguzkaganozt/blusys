/**
 * @file ili9488.h
 * @brief ILI9488 panel hardware defaults.
 *
 * Provides panel-specific constants and a default ::blusys_lcd_config_t
 * pre-filled for a 320×480 ILI9488 SPI panel in landscape orientation
 * (480 wide × 320 tall, achieved via `swap_xy` + `mirror_x`).
 *
 * Pin fields are left at `-1` (unspecified) — callers must set them for their
 * board. Every other field reflects the hardware datasheet.
 *
 * Large panels are memory- and bandwidth-sensitive; this profile uses a
 * conservative SPI clock. Validate flicker-free operation and DMA sizing on
 * your hardware before shipping.
 */

#ifndef BLUSYS_DRIVERS_PANELS_ILI9488_H
#define BLUSYS_DRIVERS_PANELS_ILI9488_H

#include "blusys/drivers/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Native panel width in pixels (portrait). */
#define BLUSYS_ILI9488_NATIVE_WIDTH  320u
/** @brief Native panel height in pixels (portrait). */
#define BLUSYS_ILI9488_NATIVE_HEIGHT 480u

/** @brief Logical width after landscape rotation (`swap_xy` + `mirror_x`). */
#define BLUSYS_ILI9488_WIDTH  480u
/** @brief Logical height after landscape rotation. */
#define BLUSYS_ILI9488_HEIGHT 320u

/** @brief Conservative SPI pixel clock sized for the panel's bandwidth. */
#define BLUSYS_ILI9488_PCLK_HZ 26000000u

/**
 * @brief Return a ::blusys_lcd_config_t pre-filled for a landscape ILI9488 panel.
 *
 * Pin fields are `-1`; the caller must fill `sclk_pin`, `mosi_pin`, `cs_pin`,
 * `dc_pin`, and optionally `rst_pin` / `bl_pin` before passing the struct to
 * `blusys_lcd_open()`.
 */
static inline blusys_lcd_config_t blusys_ili9488_default_config(void)
{
    blusys_lcd_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.driver         = BLUSYS_LCD_DRIVER_ILI9488;
    cfg.width          = BLUSYS_ILI9488_WIDTH;
    cfg.height         = BLUSYS_ILI9488_HEIGHT;
    cfg.bits_per_pixel = 16;
    cfg.bgr_order      = false;
    cfg.swap_xy        = true;
    cfg.mirror_x       = true;
    cfg.mirror_y       = false;
    cfg.invert_color   = false;

    cfg.spi.pclk_hz  = BLUSYS_ILI9488_PCLK_HZ;
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

#endif /* BLUSYS_DRIVERS_PANELS_ILI9488_H */
