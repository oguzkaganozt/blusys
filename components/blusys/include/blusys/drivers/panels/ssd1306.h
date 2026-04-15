#ifndef BLUSYS_DRIVERS_PANELS_SSD1306_H
#define BLUSYS_DRIVERS_PANELS_SSD1306_H

/* SSD1306 panel hardware defaults.
 *
 * Provides panel-specific constants and default blusys_lcd_config_t values
 * for 128x64 and 128x32 SSD1306 I2C OLED modules (monochrome, page format).
 *
 * Pin fields are left at -1 (unspecified) — callers must set sda_pin and
 * scl_pin for their board.
 *
 * Hardware notes:
 * - I2C address: common modules use 0x3C; some use 0x3D.
 * - SH1106-based modules are not the same controller; they need a
 *   dedicated profile.
 * - Some clones need invert_color = true for correct contrast. */

#include "blusys/drivers/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Full-height variant (0.96" common module). */
#define BLUSYS_SSD1306_WIDTH    128u
#define BLUSYS_SSD1306_HEIGHT   64u

/* Half-height compact variant. */
#define BLUSYS_SSD1306_HEIGHT32 32u

/* Default I2C address (most modules). */
#define BLUSYS_SSD1306_I2C_ADDR   0x3Cu
#define BLUSYS_SSD1306_I2C_FREQ_HZ 400000u

static inline blusys_lcd_config_t blusys_ssd1306_128x64_default_config(void)
{
    blusys_lcd_config_t cfg = {(blusys_lcd_driver_t)0};
    cfg.driver         = BLUSYS_LCD_DRIVER_SSD1306;
    cfg.width          = BLUSYS_SSD1306_WIDTH;
    cfg.height         = BLUSYS_SSD1306_HEIGHT;
    cfg.bits_per_pixel = 1;
    cfg.bgr_order      = false;
    cfg.swap_xy        = false;
    cfg.mirror_x       = false;
    cfg.mirror_y       = false;
    cfg.invert_color   = false;

    cfg.i2c.port     = 0;
    cfg.i2c.dev_addr = BLUSYS_SSD1306_I2C_ADDR;
    cfg.i2c.freq_hz  = BLUSYS_SSD1306_I2C_FREQ_HZ;
    cfg.i2c.rst_pin  = -1;
    cfg.i2c.sda_pin  = -1;
    cfg.i2c.scl_pin  = -1;
    return cfg;
}

static inline blusys_lcd_config_t blusys_ssd1306_128x32_default_config(void)
{
    blusys_lcd_config_t cfg = blusys_ssd1306_128x64_default_config();
    cfg.height = BLUSYS_SSD1306_HEIGHT32;
    return cfg;
}

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_DRIVERS_PANELS_SSD1306_H */
