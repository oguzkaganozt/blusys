// oled_encoder_basic_main.cpp — TE-style Pomodoro: arc ring + split panel
//
// Hardware (ESP32 defaults):
//   SSD1306 128x32 I2C:  SDA=21  SCL=22  addr=0x3C  @ 400 kHz
//   Rotary encoder:      CLK=19  DT=18   SW=23
//
// Layout on 128×32:
//
//   ┌──────────────┬───────────────────────────────┐
//   │   ╭──────╮   │                               │
//   │  (  · blink) │          25:00                │
//   │   ╰──────╯   │                               │
//   └──────────────┴───────────────────────────────┘
//    ←  32px arc →  1px  ←    95px time label    →
//
//   IDLE    — encoder sets duration, arc shows set/60 min proportion
//   RUNNING — arc drains clockwise (smooth 900 ms per tick), dot blinks
//   Done    — arc empties, returns to IDLE
//
//   Controls:
//     IDLE    CW/CCW → set duration (1-60 min, 1 min per detent)
//             Press  → start countdown
//     RUNNING Press  → stop + reset

#include <algorithm>
#include <atomic>
#include <cstdint>

#include "blusys/blusys.h"
#include "blusys/blusys.h"
#include "blusys/drivers/encoder.h"
#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

namespace {

constexpr const char *kTag        = "pomodoro";
constexpr int32_t    kW           = 128;
constexpr int32_t    kH           = 32;
constexpr int        kDefaultSecs = 25 * 60;
constexpr int        kMinSecs     =  1 * 60;
constexpr int        kMaxSecs     = 60 * 60;
constexpr int        kStepSecs    =  1 * 60;

enum class State : uint8_t { IDLE, RUNNING };

std::atomic<State> s_state{State::IDLE};
std::atomic<int>   s_set_secs{kDefaultSecs};
std::atomic<int>   s_remaining{kDefaultSecs};

struct AppCtx {
    blusys_display_t *ui;
    lv_obj_t    *arc;          /* ring progress — left panel */
    lv_obj_t    *center_dot;   /* 3×3 blink indicator at arc center */
    lv_obj_t    *time_label;   /* MM:SS — right panel */
};

/* ---- LVGL animation exec callback (must be file-scope) ---------------- */
static void arc_anim_exec(void *var, int32_t v)
{
    lv_arc_set_value(static_cast<lv_obj_t *>(var), static_cast<int16_t>(v));
}

/* ---- helpers ---------------------------------------------------------- */
static void dot_draw(lv_obj_t *dot, State state, bool blink)
{
    /* IDLE    → always visible (steady solid square)  */
    /* RUNNING → blinks on/off every 500 ms            */
    bool visible = (state == State::IDLE) || blink;
    lv_obj_set_style_bg_opa(dot, visible ? LV_OPA_COVER : LV_OPA_0, 0);
}

/* ---- encoder callback ------------------------------------------------- */
void encoder_cb(blusys_encoder_t * /*enc*/, blusys_encoder_event_t ev,
                int /*pos*/, void * /*ctx*/)
{
    auto state = s_state.load(std::memory_order_relaxed);

    if (state == State::IDLE) {
        int cur = s_set_secs.load(std::memory_order_relaxed);
        switch (ev) {
        case BLUSYS_ENCODER_EVENT_CW:
            cur = std::min(kMaxSecs, cur + kStepSecs);
            s_set_secs.store(cur, std::memory_order_relaxed);
            s_remaining.store(cur, std::memory_order_relaxed);
            break;
        case BLUSYS_ENCODER_EVENT_CCW:
            cur = std::max(kMinSecs, cur - kStepSecs);
            s_set_secs.store(cur, std::memory_order_relaxed);
            s_remaining.store(cur, std::memory_order_relaxed);
            break;
        case BLUSYS_ENCODER_EVENT_PRESS:
            ESP_LOGI(kTag, "start — %d min", cur / 60);
            s_state.store(State::RUNNING, std::memory_order_relaxed);
            break;
        default:
            break;
        }
    } else {
        if (ev == BLUSYS_ENCODER_EVENT_PRESS) {
            ESP_LOGI(kTag, "stopped");
            s_remaining.store(s_set_secs.load(std::memory_order_relaxed),
                              std::memory_order_relaxed);
            s_state.store(State::IDLE, std::memory_order_relaxed);
        }
    }
}

/* ---- app task --------------------------------------------------------- */
void app_task(void *arg)
{
    auto *ctx = static_cast<AppCtx *>(arg);

    int64_t last_tick_us  = 0;
    int64_t last_blink_us = 0;
    bool    blink_state   = true;
    State   last_state    = State::IDLE;
    int     last_secs     = -1;
    int     last_arc_val  = -1;

    while (true) {
        auto state  = s_state.load(std::memory_order_relaxed);
        auto set_s  = s_set_secs.load(std::memory_order_relaxed);
        auto remain = s_remaining.load(std::memory_order_relaxed);

        /* Countdown — drift-free 1 s ticks */
        if (state == State::RUNNING) {
            int64_t now = esp_timer_get_time();
            if (last_tick_us == 0) last_tick_us = now;
            while (now - last_tick_us >= 1'000'000LL) {
                last_tick_us += 1'000'000LL;
                remain = std::max(0, remain - 1);
                s_remaining.store(remain, std::memory_order_relaxed);
                if (remain == 0) {
                    ESP_LOGI(kTag, "done");
                    s_remaining.store(s_set_secs.load(std::memory_order_relaxed),
                                      std::memory_order_relaxed);
                    s_state.store(State::IDLE, std::memory_order_relaxed);
                    state = State::IDLE;
                    last_tick_us = 0;
                    break;
                }
            }
        } else {
            last_tick_us = 0;
        }

        /* Blink — 500 ms period while RUNNING */
        bool force_redraw = false;
        if (state == State::RUNNING) {
            int64_t now = esp_timer_get_time();
            if (last_blink_us == 0) last_blink_us = now;
            if (now - last_blink_us >= 500'000LL) {
                last_blink_us += 500'000LL;
                blink_state   = !blink_state;
                force_redraw  = true;
            }
        } else {
            blink_state   = true;
            last_blink_us = 0;
        }

        /* What the UI should show */
        int display_secs = (state == State::RUNNING) ? remain : set_s;
        int new_arc_val  = (state == State::RUNNING)
                           ? (set_s > 0 ? remain * 100 / set_s : 0)
                           : (set_s * 100 / kMaxSecs);

        bool time_changed = (state != last_state || display_secs != last_secs || force_redraw);
        bool arc_changed  = (new_arc_val != last_arc_val || state != last_state);

        if (time_changed || arc_changed) {
            int mins = display_secs / 60;
            int secs = display_secs % 60;

            blusys_display_lock(ctx->ui);

            /* Time label — bracketed, blinking colon while running */
            lv_label_set_text_fmt(ctx->time_label, "[%02d%s%02d]",
                                  mins, blink_state ? ":" : " ", secs);

            /* Center dot */
            dot_draw(ctx->center_dot, state, blink_state);

            /* Arc ring */
            if (arc_changed) {
                if (state == State::RUNNING) {
                    /* Smooth 900 ms animation per second tick */
                    lv_anim_del(ctx->arc, arc_anim_exec);
                    lv_anim_t a;
                    lv_anim_init(&a);
                    lv_anim_set_var(&a, ctx->arc);
                    lv_anim_set_exec_cb(&a, arc_anim_exec);
                    lv_anim_set_values(&a, lv_arc_get_value(ctx->arc), new_arc_val);
                    lv_anim_set_duration(&a, 900);
                    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
                    lv_anim_start(&a);
                } else {
                    /* IDLE: snap instantly, cancel any running animation */
                    lv_anim_del(ctx->arc, arc_anim_exec);
                    lv_arc_set_value(ctx->arc, new_arc_val);
                }
            }

            blusys_display_unlock(ctx->ui);

            last_state   = state;
            last_secs    = display_secs;
            last_arc_val = new_arc_val;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

}  // namespace

extern "C" void app_main(void)
{
    /* 1. LCD */
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

    /* 2. UI */
    blusys_display_t       *ui     = nullptr;
    blusys_display_config_t ui_cfg = {
        .lcd        = lcd,
        .panel_kind = BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE,
    };
    err = blusys_display_open(&ui_cfg, &ui);
    if (err != BLUSYS_OK) {
        ESP_LOGE(kTag, "ui_open: %s", blusys_err_string(err));
        blusys_lcd_close(lcd);
        return;
    }

    /* 3. Encoder */
    blusys_encoder_t       *enc     = nullptr;
    blusys_encoder_config_t enc_cfg = {
        .clk_pin          = 19,
        .dt_pin           = 18,
        .sw_pin           = 23,
        .glitch_filter_ns = 1000,
        .debounce_ms      = 50,
        .long_press_ms    = 0,
        .steps_per_detent = 0,
    };
    err = blusys_encoder_open(&enc_cfg, &enc);
    if (err != BLUSYS_OK) {
        ESP_LOGE(kTag, "encoder_open: %s", blusys_err_string(err));
        blusys_display_close(ui);
        blusys_lcd_close(lcd);
        return;
    }
    blusys_encoder_set_callback(enc, encoder_cb, nullptr);

    /* 4. Build screen --------------------------------------------------- */
    blusys_display_lock(ui);

    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, 0);

    /* Arc ring — 28×28, centered in the left 32px zone (x=2..29, y=2..29) */
    lv_obj_t *arc = lv_arc_create(screen);
    lv_obj_set_size(arc, 28, 28);
    lv_obj_set_pos(arc, 2, 2);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(arc, LV_OPA_0, 0);          /* transparent bg */
    lv_obj_set_style_pad_all(arc, 0, 0);

    lv_arc_set_rotation(arc, 270);                        /* start at top   */
    lv_arc_set_bg_angles(arc, 0, 360);                    /* full bg circle */
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, kDefaultSecs * 100 / kMaxSecs); /* 25 min ≈ 41%  */

    /* Background (unfilled) arc — thin, white */
    lv_obj_set_style_arc_color(arc, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 1, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);

    /* Indicator (filled) arc — thick, white */
    lv_obj_set_style_arc_color(arc, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);

    /* Knob — hidden */
    lv_obj_set_style_opa(arc, LV_OPA_0, LV_PART_KNOB);

    /* Cardinal tick marks — 12/3/6/9 o'clock dots just outside the arc */
    static const struct { int x, y, w, h; } kTicks[] = {
        { 15,  0, 2, 1 },   /* 12 o'clock — top    */
        { 31, 15, 1, 2 },   /*  3 o'clock — right  */
        { 15, 31, 2, 1 },   /*  6 o'clock — bottom */
        {  0, 15, 1, 2 },   /*  9 o'clock — left   */
    };
    for (auto &t : kTicks) {
        lv_obj_t *tick = lv_obj_create(screen);
        lv_obj_set_size(tick, t.w, t.h);
        lv_obj_set_pos(tick, t.x, t.y);
        lv_obj_set_style_radius(tick, 0, 0);
        lv_obj_set_style_bg_color(tick, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tick, 0, 0);
        lv_obj_set_style_pad_all(tick, 0, 0);
        lv_obj_remove_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Center dot — 2×2 solid square at arc center, blinks while RUNNING */
    lv_obj_t *center_dot = lv_obj_create(screen);
    lv_obj_set_size(center_dot, 2, 2);
    lv_obj_set_pos(center_dot, 15, 15);   /* arc center = (16,16), 2×2 → (15,15) */
    lv_obj_set_style_radius(center_dot, 0, 0);
    lv_obj_set_style_bg_color(center_dot, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(center_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(center_dot, 0, 0);
    lv_obj_set_style_pad_all(center_dot, 0, 0);
    lv_obj_remove_flag(center_dot, LV_OBJ_FLAG_SCROLLABLE);

    /* Dotted vertical divider — 3 small dots at x=32 instead of a full line */
    static const int kDividerY[] = { 8, 15, 22 };
    for (int dy : kDividerY) {
        lv_obj_t *dot = lv_obj_create(screen);
        lv_obj_set_size(dot, 1, 2);
        lv_obj_set_pos(dot, 32, dy);
        lv_obj_set_style_radius(dot, 0, 0);
        lv_obj_set_style_bg_color(dot, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Right panel container — 95×32 at x=33 */
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_set_size(right, 95, 32);
    lv_obj_set_pos(right, 33, 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_0, 0);
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_pad_all(right, 0, 0);
    lv_obj_remove_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    /* Time label — montserrat_20, bracketed, centered in right panel */
    lv_obj_t *time_label = lv_label_create(right);
    lv_label_set_text(time_label, "[25:00]");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);

    lv_screen_load(screen);
    blusys_display_unlock(ui);

    ESP_LOGI(kTag, "pomodoro ready — rotate to set, press to start");

    static AppCtx ctx = {ui, arc, center_dot, time_label};
    xTaskCreate(app_task, "pomodoro", 4096, &ctx, 4, nullptr);
}
