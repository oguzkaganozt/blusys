#ifndef BLUSYS_LCD_H
#define BLUSYS_LCD_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_lcd blusys_lcd_t;

typedef enum {
    BLUSYS_LCD_DRIVER_ST7789  = 0,
    BLUSYS_LCD_DRIVER_SSD1306 = 1,
    BLUSYS_LCD_DRIVER_NT35510 = 2,
    BLUSYS_LCD_DRIVER_ST7735  = 3,
} blusys_lcd_driver_t;

typedef struct {
    int      bus;       /* SPI host: 0 = SPI2_HOST, 1 = SPI3_HOST */
    int      sclk_pin;
    int      mosi_pin;
    int      cs_pin;
    int      dc_pin;    /* Data/Command select */
    int      rst_pin;   /* -1 if not connected */
    int      bl_pin;    /* Backlight GPIO, -1 if not used */
    uint32_t pclk_hz;
    int      x_offset;  /* GRAM column offset (default 0) */
    int      y_offset;  /* GRAM row offset (default 0) */
} blusys_lcd_spi_config_t;

typedef struct {
    int      port;      /* I2C port number */
    int      sda_pin;
    int      scl_pin;
    uint8_t  dev_addr;  /* 0x3C or 0x3D */
    int      rst_pin;   /* -1 if not connected */
    uint32_t freq_hz;
} blusys_lcd_i2c_config_t;

typedef struct {
    blusys_lcd_driver_t driver;
    uint32_t            width;
    uint32_t            height;
    uint8_t             bits_per_pixel; /* 16 = RGB565, 1 = SSD1306 mono */
    bool                bgr_order;
    union {
        blusys_lcd_spi_config_t spi;
        blusys_lcd_i2c_config_t i2c;
    };
} blusys_lcd_config_t;

blusys_err_t blusys_lcd_open(const blusys_lcd_config_t *config,
                             blusys_lcd_t **out_lcd);
blusys_err_t blusys_lcd_close(blusys_lcd_t *lcd);
blusys_err_t blusys_lcd_draw_bitmap(blusys_lcd_t *lcd,
                                    int x_start, int y_start,
                                    int x_end, int y_end,
                                    const void *color_data);
blusys_err_t blusys_lcd_on(blusys_lcd_t *lcd);
blusys_err_t blusys_lcd_off(blusys_lcd_t *lcd);
blusys_err_t blusys_lcd_mirror(blusys_lcd_t *lcd, bool mirror_x, bool mirror_y);
blusys_err_t blusys_lcd_swap_xy(blusys_lcd_t *lcd, bool swap);
blusys_err_t blusys_lcd_invert_colors(blusys_lcd_t *lcd, bool invert);
blusys_err_t blusys_lcd_set_brightness(blusys_lcd_t *lcd, int percent);
blusys_err_t blusys_lcd_get_dimensions(blusys_lcd_t *lcd,
                                       uint32_t *width, uint32_t *height);

#ifdef __cplusplus
}
#endif

#endif
