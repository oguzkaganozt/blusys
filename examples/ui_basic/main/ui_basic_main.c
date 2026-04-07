#include <stdio.h>

#include "blusys/blusys_all.h"
#include "lvgl.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_WIDTH  128
#define LCD_HEIGHT 160

void app_main(void)
{
    blusys_lcd_t *lcd = NULL;
    blusys_ui_t  *ui  = NULL;
    blusys_err_t err;

    printf("blusys ui_basic\n");
    printf("target: %s\n", blusys_target_name());

    /* 1. Open the LCD (hardware init) */
    blusys_lcd_config_t lcd_config = {
        .driver         = BLUSYS_LCD_DRIVER_ST7735,
        .width          = LCD_WIDTH,
        .height         = LCD_HEIGHT,
        .bits_per_pixel = 16,
        .bgr_order      = false,
        .spi = {
            .bus      = CONFIG_BLUSYS_UI_SPI_BUS,
            .sclk_pin = CONFIG_BLUSYS_UI_SCLK_PIN,
            .mosi_pin = CONFIG_BLUSYS_UI_MOSI_PIN,
            .cs_pin   = CONFIG_BLUSYS_UI_CS_PIN,
            .dc_pin   = CONFIG_BLUSYS_UI_DC_PIN,
            .rst_pin  = CONFIG_BLUSYS_UI_RST_PIN,
            .bl_pin   = CONFIG_BLUSYS_UI_BL_PIN,
            .pclk_hz  = CONFIG_BLUSYS_UI_PCLK_HZ,
            .x_offset = 2,
            .y_offset = 1,
        },
    };

    err = blusys_lcd_open(&lcd_config, &lcd);
    if (err != BLUSYS_OK) {
        printf("lcd_open error: %s\n", blusys_err_string(err));
        return;
    }
    printf("lcd_open: ok\n");

    /* 2. Open the UI (LVGL init + render task) */
    blusys_ui_config_t ui_config = {
        .lcd = lcd,
    };

    err = blusys_ui_open(&ui_config, &ui);
    if (err != BLUSYS_OK) {
        printf("ui_open error: %s\n", blusys_err_string(err));
        blusys_lcd_close(lcd);
        return;
    }
    printf("ui_open: ok\n");

    /* 3. Create LVGL widgets — lock required for thread safety */
    blusys_ui_lock(ui);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "BluPanda");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);

    blusys_ui_unlock(ui);
    printf("ui ready\n");

    /* Keep running — LVGL render task handles updates in background */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
