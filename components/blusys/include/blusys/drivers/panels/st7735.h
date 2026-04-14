#ifndef BLUSYS_DRIVERS_PANELS_ST7735_H
#define BLUSYS_DRIVERS_PANELS_ST7735_H

/* ST7735 panel hardware defaults.
 *
 * Provides panel-specific constants and a default blusys_lcd_config_t
 * pre-filled for a 128x160 ST7735 SPI panel in landscape orientation
 * (160 wide x 128 tall, achieved via swap_xy + mirror_x).
 *
 * Pin fields are left at -1 (unspecified) — callers must set them for
 * their board.
 *
 * Note: many ST7735 modules require non-zero x_offset / y_offset to
 * avoid a colored border bar — override if needed for your specific module. */

#include "blusys/drivers/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Native panel resolution (portrait). */
#define BLUSYS_ST7735_NATIVE_WIDTH  128u
#define BLUSYS_ST7735_NATIVE_HEIGHT 160u

/* Logical resolution after landscape rotation (swap_xy + mirror_x). */
#define BLUSYS_ST7735_WIDTH  160u
#define BLUSYS_ST7735_HEIGHT 128u

/* Typical SPI clock for ST7735. */
#define BLUSYS_ST7735_PCLK_HZ 16000000u

/* Common viewport offsets for 1.8" ST7735 modules.
 * Override to 0,0 if your module does not need them. */
#define BLUSYS_ST7735_X_OFFSET 2u
#define BLUSYS_ST7735_Y_OFFSET 1u

static inline blusys_lcd_config_t blusys_st7735_default_config(void)
{
    blusys_lcd_config_t cfg = {0};
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
