/**
 * @file ssd1306.h
 * @brief SSD1306 panel hardware defaults.
 *
 * Provides panel-specific constants and default ::blusys_lcd_config_t values
 * for 128×64 and 128×32 SSD1306 I²C OLED modules (monochrome, page format).
 *
 * Pin fields are left at `-1` (unspecified) — callers must set `sda_pin` and
 * `scl_pin` for their board.
 *
 * Hardware notes:
 *   - I²C address: most modules use `0x3C`; some use `0x3D`.
 *   - SH1106-based modules are not the same controller; they need a dedicated
 *     profile.
 *   - Some clones need `invert_color = true` for correct contrast.
 */

#ifndef BLUSYS_DRIVERS_PANELS_SSD1306_H
#define BLUSYS_DRIVERS_PANELS_SSD1306_H

#include "blusys/drivers/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Display width in pixels (both variants). */
#define BLUSYS_SSD1306_WIDTH    128u
/** @brief Display height of the full-height 128×64 variant (0.96" common module). */
#define BLUSYS_SSD1306_HEIGHT   64u
/** @brief Display height of the half-height 128×32 variant. */
#define BLUSYS_SSD1306_HEIGHT32 32u

/** @brief Default I²C address used by most SSD1306 modules. */
#define BLUSYS_SSD1306_I2C_ADDR   0x3Cu
/** @brief Default I²C bus clock. */
#define BLUSYS_SSD1306_I2C_FREQ_HZ 400000u

/**
 * @brief Return a ::blusys_lcd_config_t pre-filled for a 128×64 SSD1306 module.
 *
 * Pin fields are `-1`; the caller must fill `sda_pin` and `scl_pin` (and
 * optionally `rst_pin`) before passing the struct to `blusys_lcd_open()`.
 */
static inline blusys_lcd_config_t blusys_ssd1306_128x64_default_config(void)
{
    blusys_lcd_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
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

/** @brief Return a ::blusys_lcd_config_t pre-filled for a 128×32 SSD1306 module. */
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
