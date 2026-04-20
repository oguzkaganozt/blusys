/**
 * @file ili9341.h
 * @brief ILI9341 panel hardware defaults.
 *
 * Provides panel-specific constants and a default ::blusys_lcd_config_t
 * pre-filled for a 240×320 ILI9341 SPI panel in landscape orientation
 * (320 wide × 240 tall, achieved via `swap_xy` + `mirror_x`).
 *
 * Pin fields are left at `-1` (unspecified) — callers must set them for their
 * board. Every other field reflects the hardware datasheet.
 */

#ifndef BLUSYS_DRIVERS_PANELS_ILI9341_H
#define BLUSYS_DRIVERS_PANELS_ILI9341_H

#include "blusys/drivers/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Native panel width in pixels (portrait). */
#define BLUSYS_ILI9341_NATIVE_WIDTH  240u
/** @brief Native panel height in pixels (portrait). */
#define BLUSYS_ILI9341_NATIVE_HEIGHT 320u

/** @brief Logical width after landscape rotation (`swap_xy` + `mirror_x`). */
#define BLUSYS_ILI9341_WIDTH  320u
/** @brief Logical height after landscape rotation. */
#define BLUSYS_ILI9341_HEIGHT 240u

/** @brief Default SPI pixel clock; conservative and safe for most modules. */
#define BLUSYS_ILI9341_PCLK_HZ 40000000u

/**
 * @brief Return a ::blusys_lcd_config_t pre-filled for a landscape ILI9341 panel.
 *
 * Pin fields are `-1`; the caller must fill `sclk_pin`, `mosi_pin`, `cs_pin`,
 * and `dc_pin`. `rst_pin` and `bl_pin` default to "not connected".
 */
static inline blusys_lcd_config_t blusys_ili9341_default_config(void)
{
    blusys_lcd_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.driver         = BLUSYS_LCD_DRIVER_ILI9341;
    cfg.width          = BLUSYS_ILI9341_WIDTH;
    cfg.height         = BLUSYS_ILI9341_HEIGHT;
    cfg.bits_per_pixel = 16;
    cfg.bgr_order      = false;
    cfg.swap_xy        = true;
    cfg.mirror_x       = true;
    cfg.mirror_y       = false;
    cfg.invert_color   = false;

    cfg.spi.pclk_hz  = BLUSYS_ILI9341_PCLK_HZ;
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

#endif /* BLUSYS_DRIVERS_PANELS_ILI9341_H */
