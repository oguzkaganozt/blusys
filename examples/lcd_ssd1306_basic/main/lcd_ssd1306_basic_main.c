#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "blusys/blusys.h"
#include "blusys/blusys_services.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_WIDTH  128
#define LCD_HEIGHT 32

/*
 * SSD1306 monochrome format: each byte represents 8 vertical pixels,
 * MSB at the bottom. The framebuffer is organized as (width * height / 8)
 * bytes, laid out in horizontal pages of 128 bytes each.
 * A 128x32 display has 4 pages (32 / 8).
 */
static uint8_t s_framebuf[LCD_WIDTH * LCD_HEIGHT / 8];

void app_main(void)
{
    blusys_lcd_t *lcd = NULL;
    blusys_err_t err;

    printf("blusys lcd_ssd1306_basic\n");
    printf("target: %s\n", blusys_target_name());

    blusys_lcd_config_t config = {
        .driver         = BLUSYS_LCD_DRIVER_SSD1306,
        .width          = LCD_WIDTH,
        .height         = LCD_HEIGHT,
        .bits_per_pixel = 1,
        .bgr_order      = false,
        .i2c = {
            .port     = CONFIG_BLUSYS_LCD_SSD1306_I2C_PORT,
            .sda_pin  = CONFIG_BLUSYS_LCD_SSD1306_SDA_PIN,
            .scl_pin  = CONFIG_BLUSYS_LCD_SSD1306_SCL_PIN,
            .dev_addr = CONFIG_BLUSYS_LCD_SSD1306_DEV_ADDR,
            .rst_pin  = CONFIG_BLUSYS_LCD_SSD1306_RST_PIN,
            .freq_hz  = CONFIG_BLUSYS_LCD_SSD1306_I2C_FREQ_HZ,
        },
    };

    err = blusys_lcd_open(&config, &lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_open error: %s\n", blusys_err_string(err));
        return;
    }
    printf("lcd_open: ok\n");

    /* Pattern 1: all pixels on (white) */
    memset(s_framebuf, 0xFF, sizeof(s_framebuf));
    blusys_lcd_draw_bitmap(lcd, 0, 0, LCD_WIDTH, LCD_HEIGHT, s_framebuf);
    printf("fill white\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Pattern 2: all pixels off (black) */
    memset(s_framebuf, 0x00, sizeof(s_framebuf));
    blusys_lcd_draw_bitmap(lcd, 0, 0, LCD_WIDTH, LCD_HEIGHT, s_framebuf);
    printf("fill black\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Pattern 3: horizontal stripes (alternating page rows) */
    for (int page = 0; page < LCD_HEIGHT / 8; page++) {
        uint8_t val = (page % 2 == 0) ? 0xAA : 0x55;
        memset(&s_framebuf[page * LCD_WIDTH], val, LCD_WIDTH);
    }
    blusys_lcd_draw_bitmap(lcd, 0, 0, LCD_WIDTH, LCD_HEIGHT, s_framebuf);
    printf("stripes\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    err = blusys_lcd_close(lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_close error: %s\n", blusys_err_string(err));
    }
    printf("done\n");
}
