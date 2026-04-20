/**
 * @file lcd.h
 * @brief Panel driver layer: initialises an SPI or I2C LCD and exposes
 *        a framebuffer-style `draw_bitmap` primitive.
 *
 * Higher-level rendering (LVGL) lives in `drivers/display.h`. Concrete panel
 * initialisation sequences live in `drivers/panels/`. See docs/hal/lcd.md.
 */

#ifndef BLUSYS_LCD_H
#define BLUSYS_LCD_H

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an open LCD panel. */
typedef struct blusys_lcd blusys_lcd_t;

/** @brief Supported panel driver families. */
typedef enum {
    BLUSYS_LCD_DRIVER_ST7789  = 0,   /**< ST7789 RGB565 SPI panel. */
    BLUSYS_LCD_DRIVER_SSD1306 = 1,   /**< SSD1306 1-bpp mono I2C OLED. */
    BLUSYS_LCD_DRIVER_NT35510 = 2,   /**< NT35510 SPI panel. */
    BLUSYS_LCD_DRIVER_ST7735  = 3,   /**< ST7735 RGB565 SPI panel. */
    BLUSYS_LCD_DRIVER_ILI9341 = 4,   /**< ILI9341 RGB565 SPI panel. */
    BLUSYS_LCD_DRIVER_ILI9488 = 5,   /**< ILI9488 RGB565 SPI panel. */
    BLUSYS_LCD_DRIVER_QEMU_RGB = 6,  /**< ESP-IDF QEMU virtual framebuffer (`esp_lcd_qemu_rgb`); QEMU only. */
} blusys_lcd_driver_t;

/** @brief SPI transport configuration used by SPI panel drivers. */
typedef struct {
    int      bus;        /**< SPI host: `0` = SPI2_HOST, `1` = SPI3_HOST. */
    int      sclk_pin;   /**< SPI clock GPIO pin. */
    int      mosi_pin;   /**< SPI MOSI GPIO pin. */
    int      cs_pin;     /**< Chip-select GPIO pin. */
    int      dc_pin;     /**< Data/Command select GPIO pin. */
    int      rst_pin;    /**< Panel reset GPIO pin; `-1` if not connected. */
    int      bl_pin;     /**< Backlight GPIO pin; `-1` if not used. */
    uint32_t pclk_hz;    /**< SPI pixel clock in Hz. */
    int      x_offset;   /**< GRAM column offset (default `0`). */
    int      y_offset;   /**< GRAM row offset (default `0`). */

    /* Optional LEDC PWM backlight dimming.
     *
     * When bl_ledc_enabled is false (the default for zero-initialised
     * configs) the driver controls bl_pin as a plain GPIO — backlight is
     * either fully on or fully off and blusys_lcd_set_brightness() maps
     * any non-zero value to full on.
     *
     * When bl_ledc_enabled is true the driver programs the LEDC peripheral
     * at open time and blusys_lcd_set_brightness() maps the 0–100 range
     * linearly to the PWM duty cycle. The caller must pick a timer and
     * channel that do not conflict with other LEDC users in the application.
     * bl_ledc_freq_hz = 0 defaults to 5000 Hz. */
    bool     bl_ledc_enabled;   /**< Enable LEDC PWM backlight dimming. */
    int      bl_ledc_channel;   /**< LEDC channel (0–7). */
    int      bl_ledc_timer;     /**< LEDC timer (0–3). */
    uint32_t bl_ledc_freq_hz;   /**< LEDC PWM frequency; `0` selects 5000 Hz. */
} blusys_lcd_spi_config_t;

/** @brief I2C transport configuration used by I2C panel drivers (SSD1306). */
typedef struct {
    int      port;      /**< I2C port number. */
    int      sda_pin;   /**< SDA GPIO pin. */
    int      scl_pin;   /**< SCL GPIO pin. */
    uint8_t  dev_addr;  /**< 7-bit I2C address (`0x3C` or `0x3D`). */
    int      rst_pin;   /**< Panel reset GPIO pin; `-1` if not connected. */
    uint32_t freq_hz;   /**< I2C bus frequency in Hz. */
} blusys_lcd_i2c_config_t;

/** @brief Configuration for ::blusys_lcd_open. */
typedef struct {
    blusys_lcd_driver_t driver;         /**< Panel driver family. */
    uint32_t            width;          /**< Panel width in pixels. */
    uint32_t            height;         /**< Panel height in pixels. */
    uint8_t             bits_per_pixel; /**< `16` for RGB565, `1` for SSD1306 mono. */
    bool                bgr_order;      /**< `true` when the panel uses BGR colour order. */
    bool                swap_xy;        /**< Apply axis swap during init. */
    bool                mirror_x;       /**< Apply X mirror during init. */
    bool                mirror_y;       /**< Apply Y mirror during init. */
    bool                invert_color;   /**< Apply panel colour inversion during init. */
    union {
        blusys_lcd_spi_config_t spi;    /**< Transport config for SPI panels. */
        blusys_lcd_i2c_config_t i2c;    /**< Transport config for I2C panels. */
    };
} blusys_lcd_config_t;

/**
 * @brief Initialise the panel and return a handle.
 * @param config   Panel configuration.
 * @param out_lcd  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_IO` on bus or panel init failure.
 */
blusys_err_t blusys_lcd_open(const blusys_lcd_config_t *config,
                             blusys_lcd_t **out_lcd);

/**
 * @brief Release the panel, tear down the transport, and free the handle.
 * @param lcd  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p lcd is NULL.
 */
blusys_err_t blusys_lcd_close(blusys_lcd_t *lcd);

/**
 * @brief Blit a rectangle of pre-formatted pixel data to the panel.
 *
 * The pixel layout in @p color_data must match the panel's `bits_per_pixel`
 * and colour order as configured at open time. Coordinates are inclusive on
 * the start edge and exclusive on the end edge.
 *
 * @param lcd         Handle.
 * @param x_start     Left column (inclusive).
 * @param y_start     Top row (inclusive).
 * @param x_end       Right column (exclusive).
 * @param y_end       Bottom row (exclusive).
 * @param color_data  Pixel buffer.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_lcd_draw_bitmap(blusys_lcd_t *lcd,
                                    int x_start, int y_start,
                                    int x_end, int y_end,
                                    const void *color_data);

/** @brief Power the panel display driver on. */
blusys_err_t blusys_lcd_on(blusys_lcd_t *lcd);

/** @brief Power the panel display driver off. */
blusys_err_t blusys_lcd_off(blusys_lcd_t *lcd);

/**
 * @brief Update the panel's X/Y mirror flags at runtime.
 * @param lcd       Handle.
 * @param mirror_x  Mirror along the X axis.
 * @param mirror_y  Mirror along the Y axis.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_lcd_mirror(blusys_lcd_t *lcd, bool mirror_x, bool mirror_y);

/**
 * @brief Update the panel's axis-swap flag at runtime.
 * @param lcd   Handle.
 * @param swap  Whether to swap the X and Y axes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_lcd_swap_xy(blusys_lcd_t *lcd, bool swap);

/**
 * @brief Update the panel's colour-inversion flag at runtime.
 * @param lcd     Handle.
 * @param invert  Whether to invert colour output.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_lcd_invert_colors(blusys_lcd_t *lcd, bool invert);

/**
 * @brief Set the backlight brightness as a 0–100 percentage.
 *
 * When LEDC dimming is enabled, the value is mapped linearly to PWM duty.
 * Without LEDC, any non-zero value turns the backlight fully on.
 *
 * @param lcd      Handle.
 * @param percent  Brightness percentage (0–100).
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if out of range.
 */
blusys_err_t blusys_lcd_set_brightness(blusys_lcd_t *lcd, int percent);

/**
 * @brief Read the post-init panel dimensions (accounting for axis swap).
 * @param lcd     Handle.
 * @param width   Output: panel width in pixels.
 * @param height  Output: panel height in pixels.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_lcd_get_dimensions(blusys_lcd_t *lcd,
                                       uint32_t *width, uint32_t *height);

#ifdef __cplusplus
}
#endif

#endif
