#ifndef BLUSYS_DRIVERS_PANELS_QEMU_RGB_H
#define BLUSYS_DRIVERS_PANELS_QEMU_RGB_H

/* QEMU virtual framebuffer (esp_lcd_qemu_rgb) panel defaults.
 *
 * Provides constants and a default blusys_lcd_config_t for the QEMU
 * virtual RGB display.  Logical size matches ILI9341 landscape so that
 * the same UI layouts work in both simulated and hardware targets.
 *
 * No physical pins — all pin fields are -1. */

#include "blusys/drivers/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLUSYS_QEMU_RGB_WIDTH  320u
#define BLUSYS_QEMU_RGB_HEIGHT 240u

static inline blusys_lcd_config_t blusys_qemu_rgb_default_config(void)
{
    blusys_lcd_config_t cfg = {(blusys_lcd_driver_t)0};
    cfg.driver         = BLUSYS_LCD_DRIVER_QEMU_RGB;
    cfg.width          = BLUSYS_QEMU_RGB_WIDTH;
    cfg.height         = BLUSYS_QEMU_RGB_HEIGHT;
    cfg.bits_per_pixel = 16;
    cfg.bgr_order      = false;
    cfg.swap_xy        = false;
    cfg.mirror_x       = false;
    cfg.mirror_y       = false;
    cfg.invert_color   = false;

    /* QEMU virtual display — no physical SPI pins. */
    cfg.spi.pclk_hz  = 0;
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

#endif /* BLUSYS_DRIVERS_PANELS_QEMU_RGB_H */
