#include <stdio.h>

#include "blusys/blusys.h"
#include "blusys/blusys_services.h"
#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Kconfig `bool` symbols are #defined to 1 when selected and undefined when
 * not. These fallback guards must default to 0 so that "not set" in menuconfig
 * round-trips to 0 in C, matching the user's intent. */
#ifndef CONFIG_BLUSYS_UI_SWAP_XY
#define CONFIG_BLUSYS_UI_SWAP_XY 0
#endif

#ifndef CONFIG_BLUSYS_UI_MIRROR_X
#define CONFIG_BLUSYS_UI_MIRROR_X 0
#endif

#ifndef CONFIG_BLUSYS_UI_MIRROR_Y
#define CONFIG_BLUSYS_UI_MIRROR_Y 0
#endif

#ifndef CONFIG_BLUSYS_UI_INVERT_COLOR
#define CONFIG_BLUSYS_UI_INVERT_COLOR 0
#endif

#ifndef CONFIG_BLUSYS_UI_X_OFFSET
#define CONFIG_BLUSYS_UI_X_OFFSET 0
#endif

#ifndef CONFIG_BLUSYS_UI_Y_OFFSET
#define CONFIG_BLUSYS_UI_Y_OFFSET 0
#endif

#ifndef CONFIG_BLUSYS_UI_FULL_REFRESH
#define CONFIG_BLUSYS_UI_FULL_REFRESH 0
#endif

#ifndef CONFIG_BLUSYS_UI_BUF_LINES
#define CONFIG_BLUSYS_UI_BUF_LINES 20
#endif

#if !defined(CONFIG_BLUSYS_UI_SCENE_LABEL_ONLY) && \
    !defined(CONFIG_BLUSYS_UI_SCENE_FRAME_ONLY) && \
    !defined(CONFIG_BLUSYS_UI_SCENE_FULL)
#define CONFIG_BLUSYS_UI_SCENE_FULL 1
#endif

#if CONFIG_BLUSYS_UI_SWAP_XY
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#else
#define LCD_WIDTH  128
#define LCD_HEIGHT 160
#endif

static lv_obj_t *create_debug_rect(lv_obj_t *parent, int x, int y,
                                   int width, int height, lv_color_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);

    lv_obj_remove_style_all(obj);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, width, height);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    return obj;
}

static void create_debug_overlay(lv_obj_t *scr)
{
    const int marker = 6;

    create_debug_rect(scr, 0, 0, LCD_WIDTH, 1, lv_color_white());
    create_debug_rect(scr, 0, LCD_HEIGHT - 1, LCD_WIDTH, 1, lv_color_white());
    create_debug_rect(scr, 0, 0, 1, LCD_HEIGHT, lv_color_white());
    create_debug_rect(scr, LCD_WIDTH - 1, 0, 1, LCD_HEIGHT, lv_color_white());

    create_debug_rect(scr, 0, 0, marker, marker, lv_palette_main(LV_PALETTE_RED));
    create_debug_rect(scr, LCD_WIDTH - marker, 0, marker, marker,
                      lv_palette_main(LV_PALETTE_GREEN));
    create_debug_rect(scr, 0, LCD_HEIGHT - marker, marker, marker,
                      lv_palette_main(LV_PALETTE_BLUE));
    create_debug_rect(scr, LCD_WIDTH - marker, LCD_HEIGHT - marker, marker, marker,
                      lv_palette_main(LV_PALETTE_YELLOW));
}

void app_main(void)
{
    blusys_lcd_t *lcd = NULL;
    blusys_display_t  *ui  = NULL;
    blusys_err_t err;

    printf("blusys ui_basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("buf_lines: %d\n", CONFIG_BLUSYS_UI_BUF_LINES);

    /* 1. Open the LCD (hardware init) */
    blusys_lcd_config_t lcd_config = {
        .driver         = BLUSYS_LCD_DRIVER_ST7735,
        .width          = LCD_WIDTH,
        .height         = LCD_HEIGHT,
        .bits_per_pixel = 16,
        .bgr_order      = false,
        .swap_xy        = CONFIG_BLUSYS_UI_SWAP_XY,
        .mirror_x       = CONFIG_BLUSYS_UI_MIRROR_X,
        .mirror_y       = CONFIG_BLUSYS_UI_MIRROR_Y,
        .invert_color   = CONFIG_BLUSYS_UI_INVERT_COLOR,
        .spi = {
            .bus      = CONFIG_BLUSYS_UI_SPI_BUS,
            .sclk_pin = CONFIG_BLUSYS_UI_SCLK_PIN,
            .mosi_pin = CONFIG_BLUSYS_UI_MOSI_PIN,
            .cs_pin   = CONFIG_BLUSYS_UI_CS_PIN,
            .dc_pin   = CONFIG_BLUSYS_UI_DC_PIN,
            .rst_pin  = CONFIG_BLUSYS_UI_RST_PIN,
            .bl_pin   = CONFIG_BLUSYS_UI_BL_PIN,
            .pclk_hz  = CONFIG_BLUSYS_UI_PCLK_HZ,
            .x_offset = CONFIG_BLUSYS_UI_X_OFFSET,
            .y_offset = CONFIG_BLUSYS_UI_Y_OFFSET,
        },
    };

    err = blusys_lcd_open(&lcd_config, &lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_open error: %s\n", blusys_err_string(err));
        return;
    }
    printf("lcd_open: ok\n");

    /* 2. Open the UI (LVGL init + render task) */
    blusys_display_config_t ui_config = {
        .lcd = lcd,
        .buf_lines = CONFIG_BLUSYS_UI_BUF_LINES,
        .full_refresh = CONFIG_BLUSYS_UI_FULL_REFRESH,
    };

    err = blusys_display_open(&ui_config, &ui);
    if (err != BLUSYS_OK) {
        printf("ui_open error: %s\n", blusys_err_string(err));
        blusys_lcd_close(lcd);
        return;
    }
    printf("ui_open: ok\n");

    /* 3. Create LVGL widgets — lock required for thread safety */
    blusys_display_lock(ui);

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_radius(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_outline_width(scr, 0, 0);
    lv_obj_set_style_shadow_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(scr, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

#if defined(CONFIG_BLUSYS_UI_SCENE_FRAME_ONLY) || defined(CONFIG_BLUSYS_UI_SCENE_FULL)
    create_debug_overlay(scr);
#endif

#if defined(CONFIG_BLUSYS_UI_SCENE_LABEL_ONLY) || defined(CONFIG_BLUSYS_UI_SCENE_FULL)
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "BluPanda");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
#endif

    lv_screen_load(scr);

    blusys_display_unlock(ui);
    printf("ui ready\n");

    /* Keep running — LVGL render task handles updates in background */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
