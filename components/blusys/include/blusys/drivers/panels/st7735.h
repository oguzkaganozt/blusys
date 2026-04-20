/**
 * @file st7735.h
 * @brief ST7735 panel hardware defaults.
 *
 * Provides panel-specific constants and a default ::blusys_lcd_config_t
 * pre-filled for a 128×160 ST7735 SPI panel in landscape orientation
 * (160 wide × 128 tall, achieved via `swap_xy` + `mirror_x`).
 *
 * Pin fields are left at `-1` (unspecified) — callers must set them for their
 * board.
 *
 * Many ST7735 modules require non-zero `x_offset` / `y_offset` to avoid a
 * colored border bar — override if needed for your specific module.
 */

#ifndef BLUSYS_DRIVERS_PANELS_ST7735_H
#define BLUSYS_DRIVERS_PANELS_ST7735_H

#include "blusys/drivers/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Native panel width in pixels (portrait). */
#define BLUSYS_ST7735_NATIVE_WIDTH  128u
/** @brief Native panel height in pixels (portrait). */
#define BLUSYS_ST7735_NATIVE_HEIGHT 160u

/** @brief Logical width after landscape rotation (`swap_xy` + `mirror_x`). */
#define BLUSYS_ST7735_WIDTH  160u
/** @brief Logical height after landscape rotation. */
#define BLUSYS_ST7735_HEIGHT 128u

/** @brief Typical SPI pixel clock for ST7735 modules. */
#define BLUSYS_ST7735_PCLK_HZ 16000000u

/** @brief Common viewport X offset for 1.8" ST7735 modules (override to `0` if unneeded). */
#define BLUSYS_ST7735_X_OFFSET 2u
/** @brief Common viewport Y offset for 1.8" ST7735 modules. */
#define BLUSYS_ST7735_Y_OFFSET 1u

/**
 * @brief Return a ::blusys_lcd_config_t pre-filled for a landscape ST7735 panel.
 *
 * Pin fields are `-1`; the caller must fill `sclk_pin`, `mosi_pin`, `cs_pin`,
 * `dc_pin`, and optionally `rst_pin` / `bl_pin` before passing the struct to
 * `blusys_lcd_open()`.
 */
static inline blusys_lcd_config_t blusys_st7735_default_config(void)
{
    blusys_lcd_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.driver         = BLUSYS_LCD_DRIVER_ST7735;
    cfg.width          = BLUSYS_ST7735_WIDTH;
    cfg.height         = BLUSYS_ST7735_HEIGHT;
    cfg.bits_per_pixel = 16;
    cfg.bgr_order      = false;
    cfg.swap_xy        = true;
    cfg.mirror_x       = true;
    cfg.mirror_y       = false;
    cfg.invert_color   = false;

    cfg.spi.pclk_hz  = BLUSYS_ST7735_PCLK_HZ;
    cfg.spi.x_offset = BLUSYS_ST7735_X_OFFSET;
    cfg.spi.y_offset = BLUSYS_ST7735_Y_OFFSET;
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

#endif /* BLUSYS_DRIVERS_PANELS_ST7735_H */
