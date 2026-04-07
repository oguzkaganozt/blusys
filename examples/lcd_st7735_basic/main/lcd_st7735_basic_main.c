#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "blusys/blusys_services.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_WIDTH  128
#define LCD_HEIGHT 160

static uint16_t s_row_buf[LCD_WIDTH];

static void fill_screen(blusys_lcd_t *lcd, uint16_t color)
{
    for (int i = 0; i < LCD_WIDTH; i++) {
        s_row_buf[i] = color;
    }
    for (int row = 0; row < LCD_HEIGHT; row++) {
        blusys_lcd_draw_bitmap(lcd, 0, row, LCD_WIDTH, row + 1, s_row_buf);
    }
}

void app_main(void)
{
    blusys_lcd_t *lcd = NULL;
    blusys_err_t err;

    printf("blusys lcd_st7735_basic\n");
    printf("target: %s\n", blusys_target_name());

    blusys_lcd_config_t config = {
        .driver         = BLUSYS_LCD_DRIVER_ST7735,
        .width          = LCD_WIDTH,
        .height         = LCD_HEIGHT,
        .bits_per_pixel = 16,
        .bgr_order      = false,
        .spi = {
            .bus      = CONFIG_BLUSYS_LCD_ST7735_SPI_BUS,
            .sclk_pin = CONFIG_BLUSYS_LCD_ST7735_SCLK_PIN,
            .mosi_pin = CONFIG_BLUSYS_LCD_ST7735_MOSI_PIN,
            .cs_pin   = CONFIG_BLUSYS_LCD_ST7735_CS_PIN,
            .dc_pin   = CONFIG_BLUSYS_LCD_ST7735_DC_PIN,
            .rst_pin  = CONFIG_BLUSYS_LCD_ST7735_RST_PIN,
            .bl_pin   = CONFIG_BLUSYS_LCD_ST7735_BL_PIN,
            .pclk_hz  = CONFIG_BLUSYS_LCD_ST7735_PCLK_HZ,
            .x_offset = 2,
            .y_offset = 1,
        },
    };

    err = blusys_lcd_open(&config, &lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_open error: %s\n", blusys_err_string(err));
        return;
    }
    printf("lcd_open: ok\n");

    /* Cycle through red, green, blue fills */
    const uint16_t colors[] = {0xF800u, 0x07E0u, 0x001Fu}; /* RGB565 R, G, B */
    const char *names[]     = {"red", "green", "blue"};

    for (int c = 0; c < 3; c++) {
        printf("fill %s\n", names[c]);
        fill_screen(lcd, colors[c]);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("fill complete\n");

    err = blusys_lcd_close(lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_close error: %s\n", blusys_err_string(err));
    }
    printf("done\n");
}
