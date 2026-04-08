#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "blusys/blusys_all.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Fallback defaults must be 0 for bool Kconfig symbols, because "not set" in
 * menuconfig leaves the symbol undefined rather than defining it to 0. If the
 * fallback defaulted to 1, selecting portrait orientation in menuconfig would
 * be silently ignored and the example would still compile as landscape. */
#ifndef CONFIG_BLUSYS_LCD_ST7735_SWAP_XY
#define CONFIG_BLUSYS_LCD_ST7735_SWAP_XY 0
#endif

#ifndef CONFIG_BLUSYS_LCD_ST7735_MIRROR_X
#define CONFIG_BLUSYS_LCD_ST7735_MIRROR_X 0
#endif

#ifndef CONFIG_BLUSYS_LCD_ST7735_MIRROR_Y
#define CONFIG_BLUSYS_LCD_ST7735_MIRROR_Y 0
#endif

#ifndef CONFIG_BLUSYS_LCD_ST7735_INVERT_COLOR
#define CONFIG_BLUSYS_LCD_ST7735_INVERT_COLOR 0
#endif

#ifndef CONFIG_BLUSYS_LCD_ST7735_X_OFFSET
#define CONFIG_BLUSYS_LCD_ST7735_X_OFFSET 2
#endif

#ifndef CONFIG_BLUSYS_LCD_ST7735_Y_OFFSET
#define CONFIG_BLUSYS_LCD_ST7735_Y_OFFSET 1
#endif

#if CONFIG_BLUSYS_LCD_ST7735_SWAP_XY
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#else
#define LCD_WIDTH  128
#define LCD_HEIGHT 160
#endif

/* --------------------------------------------------------------------------
 * Minimal 5x7 ASCII font (printable chars 0x20–0x7E).
 * Each character is 5 bytes wide; bit 0 of each byte = top pixel of column.
 * -------------------------------------------------------------------------- */
#define FONT_W 5
#define FONT_H 7
#define FONT_FIRST 0x20
#define LCD_FILL_BAND_LINES 40

static const uint8_t s_font5x7[][FONT_W] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' */
    {0x00,0x00,0x5F,0x00,0x00}, /* '!' */
    {0x00,0x07,0x00,0x07,0x00}, /* '"' */
    {0x14,0x7F,0x14,0x7F,0x14}, /* '#' */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* '$' */
    {0x23,0x13,0x08,0x64,0x62}, /* '%' */
    {0x36,0x49,0x55,0x22,0x50}, /* '&' */
    {0x00,0x05,0x03,0x00,0x00}, /* ''' */
    {0x00,0x1C,0x22,0x41,0x00}, /* '(' */
    {0x00,0x41,0x22,0x1C,0x00}, /* ')' */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* '*' */
    {0x08,0x08,0x3E,0x08,0x08}, /* '+' */
    {0x00,0x50,0x30,0x00,0x00}, /* ',' */
    {0x08,0x08,0x08,0x08,0x08}, /* '-' */
    {0x00,0x60,0x60,0x00,0x00}, /* '.' */
    {0x20,0x10,0x08,0x04,0x02}, /* '/' */
    {0x3E,0x51,0x49,0x45,0x3E}, /* '0' */
    {0x00,0x42,0x7F,0x40,0x00}, /* '1' */
    {0x42,0x61,0x51,0x49,0x46}, /* '2' */
    {0x21,0x41,0x45,0x4B,0x31}, /* '3' */
    {0x18,0x14,0x12,0x7F,0x10}, /* '4' */
    {0x27,0x45,0x45,0x45,0x39}, /* '5' */
    {0x3C,0x4A,0x49,0x49,0x30}, /* '6' */
    {0x01,0x71,0x09,0x05,0x03}, /* '7' */
    {0x36,0x49,0x49,0x49,0x36}, /* '8' */
    {0x06,0x49,0x49,0x29,0x1E}, /* '9' */
    {0x00,0x36,0x36,0x00,0x00}, /* ':' */
    {0x00,0x56,0x36,0x00,0x00}, /* ';' */
    {0x00,0x08,0x14,0x22,0x41}, /* '<' */
    {0x14,0x14,0x14,0x14,0x14}, /* '=' */
    {0x41,0x22,0x14,0x08,0x00}, /* '>' */
    {0x02,0x01,0x51,0x09,0x06}, /* '?' */
    {0x32,0x49,0x79,0x41,0x3E}, /* '@' */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 'A' */
    {0x7F,0x49,0x49,0x49,0x36}, /* 'B' */
    {0x3E,0x41,0x41,0x41,0x22}, /* 'C' */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 'D' */
    {0x7F,0x49,0x49,0x49,0x41}, /* 'E' */
    {0x7F,0x09,0x09,0x09,0x01}, /* 'F' */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 'G' */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 'H' */
    {0x00,0x41,0x7F,0x41,0x00}, /* 'I' */
    {0x20,0x40,0x41,0x3F,0x01}, /* 'J' */
    {0x7F,0x08,0x14,0x22,0x41}, /* 'K' */
    {0x7F,0x40,0x40,0x40,0x40}, /* 'L' */
    {0x7F,0x02,0x04,0x02,0x7F}, /* 'M' */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 'N' */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 'O' */
    {0x7F,0x09,0x09,0x09,0x06}, /* 'P' */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 'Q' */
    {0x7F,0x09,0x19,0x29,0x46}, /* 'R' */
    {0x46,0x49,0x49,0x49,0x31}, /* 'S' */
    {0x01,0x01,0x7F,0x01,0x01}, /* 'T' */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 'U' */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 'V' */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 'W' */
    {0x63,0x14,0x08,0x14,0x63}, /* 'X' */
    {0x07,0x08,0x70,0x08,0x07}, /* 'Y' */
    {0x61,0x51,0x49,0x45,0x43}, /* 'Z' */
    {0x00,0x7F,0x41,0x41,0x00}, /* '[' */
    {0x02,0x04,0x08,0x10,0x20}, /* '\' */
    {0x00,0x41,0x41,0x7F,0x00}, /* ']' */
    {0x04,0x02,0x01,0x02,0x04}, /* '^' */
    {0x40,0x40,0x40,0x40,0x40}, /* '_' */
    {0x00,0x01,0x02,0x04,0x00}, /* '`' */
    {0x20,0x54,0x54,0x54,0x78}, /* 'a' */
    {0x7F,0x48,0x44,0x44,0x38}, /* 'b' */
    {0x38,0x44,0x44,0x44,0x20}, /* 'c' */
    {0x38,0x44,0x44,0x48,0x7F}, /* 'd' */
    {0x38,0x54,0x54,0x54,0x18}, /* 'e' */
    {0x08,0x7E,0x09,0x01,0x02}, /* 'f' */
    {0x08,0x54,0x54,0x54,0x3C}, /* 'g' */
    {0x7F,0x08,0x04,0x04,0x78}, /* 'h' */
    {0x00,0x44,0x7D,0x40,0x00}, /* 'i' */
    {0x20,0x40,0x44,0x3D,0x00}, /* 'j' */
    {0x7F,0x10,0x28,0x44,0x00}, /* 'k' */
    {0x00,0x41,0x7F,0x40,0x00}, /* 'l' */
    {0x7C,0x04,0x18,0x04,0x78}, /* 'm' */
    {0x7C,0x08,0x04,0x04,0x78}, /* 'n' */
    {0x38,0x44,0x44,0x44,0x38}, /* 'o' */
    {0x7C,0x14,0x14,0x14,0x08}, /* 'p' */
    {0x08,0x14,0x14,0x18,0x7C}, /* 'q' */
    {0x7C,0x08,0x04,0x04,0x08}, /* 'r' */
    {0x48,0x54,0x54,0x54,0x20}, /* 's' */
    {0x04,0x3F,0x44,0x40,0x20}, /* 't' */
    {0x3C,0x40,0x40,0x40,0x7C}, /* 'u' */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 'v' */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 'w' */
    {0x44,0x28,0x10,0x28,0x44}, /* 'x' */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 'y' */
    {0x44,0x64,0x54,0x4C,0x44}, /* 'z' */
    {0x00,0x08,0x36,0x41,0x00}, /* '{' */
    {0x00,0x00,0x7F,0x00,0x00}, /* '|' */
    {0x00,0x41,0x36,0x08,0x00}, /* '}' */
    {0x08,0x08,0x2A,0x1C,0x08}, /* '~' */
};

static void draw_string(blusys_lcd_t *lcd, int x, int y, const char *str,
                        uint16_t fg, uint16_t bg, int scale)
{
    size_t len = strlen(str);
    int char_w = (FONT_W + 1) * scale; /* +1 pixel gap between chars */
    int text_w = (len > 0u) ? ((int)len * char_w) - scale : 0;
    int text_h = FONT_H * scale;
    size_t pixel_count;
    uint16_t *text_buf;

    if (text_w <= 0 || text_h <= 0) {
        return;
    }

    pixel_count = (size_t)text_w * (size_t)text_h;
    text_buf = malloc(pixel_count * sizeof(*text_buf));
    if (text_buf == NULL) {
        return;
    }

    for (size_t i = 0; i < pixel_count; i++) {
        text_buf[i] = bg;
    }

    for (size_t index = 0; index < len; index++) {
        char ch = str[index];
        int glyph_x = (int)index * char_w;

        if (ch < FONT_FIRST || ch > (char)(FONT_FIRST + sizeof(s_font5x7) / FONT_W - 1)) {
            ch = ' ';
        }

        const uint8_t *glyph = s_font5x7[(uint8_t)ch - FONT_FIRST];

        for (int row = 0; row < text_h; row++) {
            int bit_row = row / scale;
            uint16_t *dst = text_buf + ((size_t)row * text_w) + glyph_x;

            for (int col = 0; col < FONT_W * scale; col++) {
                int bit_col = col / scale;
                uint8_t set = (glyph[bit_col] >> bit_row) & 0x01;

                if (set) {
                    dst[col] = fg;
                }
            }
        }
    }

    blusys_lcd_draw_bitmap(lcd, x, y, x + text_w, y + text_h, text_buf);
    free(text_buf);
}

/* -------------------------------------------------------------------------- */

static uint16_t s_fill_buf[LCD_WIDTH * LCD_FILL_BAND_LINES];

static void fill_screen(blusys_lcd_t *lcd, uint16_t color)
{
    for (size_t i = 0; i < (sizeof(s_fill_buf) / sizeof(s_fill_buf[0])); i++) {
        s_fill_buf[i] = color;
    }

    for (int row = 0; row < LCD_HEIGHT; row += LCD_FILL_BAND_LINES) {
        int band_height = LCD_HEIGHT - row;

        if (band_height > LCD_FILL_BAND_LINES) {
            band_height = LCD_FILL_BAND_LINES;
        }

        blusys_lcd_draw_bitmap(lcd, 0, row, LCD_WIDTH, row + band_height, s_fill_buf);
    }
}

void app_main(void)
{
    blusys_lcd_t *lcd = NULL;
    blusys_err_t err;
    const char *title = "BluPanda";
    const int scale = 2;
    const int char_stride = (FONT_W + 1) * scale;
    const size_t title_len = strlen(title);
    const int text_width = (title_len > 0u)
                           ? ((int)title_len * char_stride) - scale
                           : 0;
    const int text_height = FONT_H * scale;
    const int text_x = (LCD_WIDTH - text_width) / 2;
    const int text_y = (LCD_HEIGHT - text_height) / 2;

    printf("blusys lcd_st7735_basic\n");
    printf("target: %s\n", blusys_target_name());

    blusys_lcd_config_t config = {
        .driver         = BLUSYS_LCD_DRIVER_ST7735,
        .width          = LCD_WIDTH,
        .height         = LCD_HEIGHT,
        .bits_per_pixel = 16,
        .bgr_order      = false,
        .swap_xy        = CONFIG_BLUSYS_LCD_ST7735_SWAP_XY,
        .mirror_x       = CONFIG_BLUSYS_LCD_ST7735_MIRROR_X,
        .mirror_y       = CONFIG_BLUSYS_LCD_ST7735_MIRROR_Y,
        .invert_color   = CONFIG_BLUSYS_LCD_ST7735_INVERT_COLOR,
        .spi = {
            .bus      = CONFIG_BLUSYS_LCD_ST7735_SPI_BUS,
            .sclk_pin = CONFIG_BLUSYS_LCD_ST7735_SCLK_PIN,
            .mosi_pin = CONFIG_BLUSYS_LCD_ST7735_MOSI_PIN,
            .cs_pin   = CONFIG_BLUSYS_LCD_ST7735_CS_PIN,
            .dc_pin   = CONFIG_BLUSYS_LCD_ST7735_DC_PIN,
            .rst_pin  = CONFIG_BLUSYS_LCD_ST7735_RST_PIN,
            .bl_pin   = CONFIG_BLUSYS_LCD_ST7735_BL_PIN,
            .pclk_hz  = CONFIG_BLUSYS_LCD_ST7735_PCLK_HZ,
            .x_offset = CONFIG_BLUSYS_LCD_ST7735_X_OFFSET,
            .y_offset = CONFIG_BLUSYS_LCD_ST7735_Y_OFFSET,
        },
    };

    err = blusys_lcd_open(&config, &lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_open error: %s\n", blusys_err_string(err));
        return;
    }
    printf("lcd_open: ok\n");

    /* Black background */
    fill_screen(lcd, 0x0000);

    /* Keep the raw drawing check aligned with the active logical panel size. */
    draw_string(lcd, text_x, text_y, title, 0xFFFF, 0x0000, scale);
    printf("drew BluPanda\n");

    vTaskDelay(pdMS_TO_TICKS(5000));

    err = blusys_lcd_close(lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_close error: %s\n", blusys_err_string(err));
    }
    printf("done\n");
}
