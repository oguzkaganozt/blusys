#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "blusys/blusys.h"
#include "blusys/blusys_services.h"
#include "sdkconfig.h"

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

static uint16_t s_row_buf[LCD_WIDTH];

void app_main(void)
{
    blusys_lcd_t *lcd = NULL;
    blusys_err_t err;

    printf("blusys lcd_basic\n");
    printf("target: %s\n", blusys_target_name());

    blusys_lcd_config_t config = {
        .driver         = BLUSYS_LCD_DRIVER_ST7789,
        .width          = LCD_WIDTH,
        .height         = LCD_HEIGHT,
        .bits_per_pixel = 16,
        .bgr_order      = false,
        .spi = {
            .bus      = CONFIG_BLUSYS_LCD_BASIC_SPI_BUS,
            .sclk_pin = CONFIG_BLUSYS_LCD_BASIC_SCLK_PIN,
            .mosi_pin = CONFIG_BLUSYS_LCD_BASIC_MOSI_PIN,
            .cs_pin   = CONFIG_BLUSYS_LCD_BASIC_CS_PIN,
            .dc_pin   = CONFIG_BLUSYS_LCD_BASIC_DC_PIN,
            .rst_pin  = CONFIG_BLUSYS_LCD_BASIC_RST_PIN,
            .bl_pin   = CONFIG_BLUSYS_LCD_BASIC_BL_PIN,
            .pclk_hz  = CONFIG_BLUSYS_LCD_BASIC_PCLK_HZ,
        },
    };

    err = blusys_lcd_open(&config, &lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_open error: %s\n", blusys_err_string(err));
        return;
    }
    printf("lcd_open: ok\n");

    /* Fill display with red (RGB565 = 0xF800) */
    for (int i = 0; i < LCD_WIDTH; i++) {
        s_row_buf[i] = 0xF800u;
    }
    for (int row = 0; row < LCD_HEIGHT; row++) {
        err = blusys_lcd_draw_bitmap(lcd, 0, row, LCD_WIDTH, row + 1, s_row_buf);
        if (err != BLUSYS_OK) {
            printf("draw_bitmap error at row %d: %s\n", row, blusys_err_string(err));
            break;
        }
    }
    printf("fill complete\n");

    err = blusys_lcd_close(lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_close error: %s\n", blusys_err_string(err));
    }
    printf("done\n");
}
