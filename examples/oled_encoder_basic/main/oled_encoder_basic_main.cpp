// oled_encoder_basic_main.cpp — SSD1306 OLED + encoder bar demo
//
// Hardware (ESP32 defaults):
//   SSD1306 128x32 I2C:  SDA=21  SCL=22  addr=0x3C
//   Rotary encoder:      CLK=19  DT=18   SW=23
//
// Behaviour:
//   Rotate CW  → bar increases  (+1 per detent, max 100)
//   Rotate CCW → bar decreases  (-1 per detent, min 0)
//   Press SW   → reset to 50
//
// Target support:
//   ESP32, ESP32-S3 — PCNT hardware encoder decoding available
//   ESP32-C3        — PCNT not supported; blusys_encoder_open returns
//                     BLUSYS_ERR_NOT_SUPPORTED and the demo exits early

#include <algorithm>
#include <atomic>
#include <cstdint>

#include "blusys/blusys.h"
#include "blusys/blusys_services.h"
#include "blusys/drivers/input/encoder.h"
#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

namespace {

constexpr const char *kTag     = "oled_enc";
constexpr int32_t    kW        = 128;
constexpr int32_t    kH        = 32;
constexpr int        kBarReset = 50;

std::atomic<int> s_val{kBarReset};

struct UiCtx {
    blusys_ui_t *ui;
    lv_obj_t    *bar;
    lv_obj_t    *label;
};

/* ---- encoder callback ------------------------------------------------- */
void encoder_cb(blusys_encoder_t * /*enc*/, blusys_encoder_event_t ev,
                int /*pos*/, void * /*ctx*/)
{
    int cur = s_val.load(std::memory_order_relaxed);

    switch (ev) {
    case BLUSYS_ENCODER_EVENT_CW:
        cur = std::min(100, cur + 1);
        break;
    case BLUSYS_ENCODER_EVENT_CCW:
        cur = std::max(0, cur - 1);
        break;
    case BLUSYS_ENCODER_EVENT_PRESS:
        cur = kBarReset;
        break;
    default:
        return;
    }

    s_val.store(cur, std::memory_order_relaxed);
}

/* ---- UI update task — polls atomic, redraws bar when value changes ---- */
void ui_update_task(void *arg)
{
    auto *ctx = static_cast<UiCtx *>(arg);
    int   last = -1;

    while (true) {
        int cur = s_val.load(std::memory_order_relaxed);

        if (cur != last) {
            blusys_ui_lock(ctx->ui);
            lv_bar_set_value(ctx->bar, cur, LV_ANIM_OFF);
            lv_label_set_text_fmt(ctx->label, "%d", cur);
            blusys_ui_unlock(ctx->ui);
            last = cur;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

}  // namespace

extern "C" void app_main(void)
{
    /* 1. Open SSD1306 over I2C */
    blusys_lcd_t       *lcd     = nullptr;
    blusys_lcd_config_t lcd_cfg = {
        .driver         = BLUSYS_LCD_DRIVER_SSD1306,
        .width          = kW,
        .height         = kH,
        .bits_per_pixel = 1,
        .bgr_order      = false,
    };
    lcd_cfg.i2c.port     = 0;
    lcd_cfg.i2c.sda_pin  = 21;
    lcd_cfg.i2c.scl_pin  = 22;
    lcd_cfg.i2c.dev_addr = 0x3C;
    lcd_cfg.i2c.rst_pin  = -1;
    lcd_cfg.i2c.freq_hz  = 400000;

    blusys_err_t err = blusys_lcd_open(&lcd_cfg, &lcd);
    if (err != BLUSYS_OK) {
        ESP_LOGE(kTag, "lcd_open: %s", blusys_err_string(err));
        return;
    }
    ESP_LOGI(kTag, "lcd_open ok");

    /* 2. Open blusys_ui in MONO_PAGE mode */
    blusys_ui_t       *ui     = nullptr;
    blusys_ui_config_t ui_cfg = {
        .lcd        = lcd,
        .panel_kind = BLUSYS_UI_PANEL_KIND_MONO_PAGE,
    };

    err = blusys_ui_open(&ui_cfg, &ui);
    if (err != BLUSYS_OK) {
        ESP_LOGE(kTag, "ui_open: %s", blusys_err_string(err));
        blusys_lcd_close(lcd);
        return;
    }
    ESP_LOGI(kTag, "ui_open ok");

    /* 3. Open encoder */
    blusys_encoder_t       *enc     = nullptr;
    blusys_encoder_config_t enc_cfg = {
        .clk_pin          = 19,
        .dt_pin           = 18,
        .sw_pin           = 23,
        .glitch_filter_ns = 1000,
        .debounce_ms      = 50,
        .long_press_ms    = 0,
        .steps_per_detent = 0,   /* default 4 */
    };

    err = blusys_encoder_open(&enc_cfg, &enc);
    if (err != BLUSYS_OK) {
        ESP_LOGE(kTag, "encoder_open: %s", blusys_err_string(err));
        blusys_ui_close(ui);
        blusys_lcd_close(lcd);
        return;
    }
    blusys_encoder_set_callback(enc, encoder_cb, nullptr);
    ESP_LOGI(kTag, "encoder_open ok (CLK=19 DT=18 SW=23)");

    /* 4. Build screen: value label (top) + progress bar (bottom) */
    blusys_ui_lock(ui);

    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, 0);

    /* Value label — top-center */
    lv_obj_t *val_label = lv_label_create(screen);
    lv_label_set_text(val_label, "50");
    lv_obj_set_style_text_color(val_label, lv_color_white(), 0);
    lv_obj_align(val_label, LV_ALIGN_TOP_MID, 0, 1);

    /* Progress bar — bottom, white fill on black, square corners */
    lv_obj_t *bar = lv_bar_create(screen);
    lv_obj_set_size(bar, 120, 8);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, kBarReset, LV_ANIM_OFF);

    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN);

    lv_obj_set_style_radius(bar, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);

    lv_screen_load(screen);
    blusys_ui_unlock(ui);

    ESP_LOGI(kTag, "oled_encoder running — rotate to move bar, press to reset");

    /* 5. UI update task — priority below render task default (5) */
    static UiCtx ctx = {ui, bar, val_label};
    xTaskCreate(ui_update_task, "ui_update", 4096, &ctx, 4, nullptr);
}
