// Bluetooth volume-controller platform layer.
//
// Wires an EC11 encoder + status LED to the shared app_logic reducer and
// exposes ble_hid_device_capability as the single outbound channel.
// The reducer stays capability-free and talks to the radio via hid_send_fn.
//
// Device features:
//   • Encoder CW/CCW → mode-aware consumer control (volume / media / brightness)
//   • Encoder short-press → context action (mute / play-pause)
//   • Encoder long-press → cycle mode (volume → media → brightness → …)
//   • Battery ADC → BLE Battery Service (optional, see Kconfig)
//   • Deep sleep after inactivity timeout, wake on encoder CLK edge

#include "core/app_logic.hpp"

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/ble_hid_device.hpp"
#include "blusys/hal/log.h"

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#include "blusys/drivers/encoder.h"
#include "blusys/hal/adc.h"
#include "blusys/hal/gpio.h"
#include "blusys/hal/sleep.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#endif

#if defined(ESP_PLATFORM) && defined(CONFIG_BTCTRL_DISPLAY_ENABLED)
#include "blusys/drivers/lcd.h"
#include "blusys/drivers/display.h"
#include "lvgl.h"
#endif

#include <atomic>
#include <cstdint>

namespace bluetooth_controller::system {

namespace {

constexpr const char *kTag = "bt_controller";

// ── Platform-specific constants ──────────────────────────────────────────────
#ifdef ESP_PLATFORM
constexpr int           kEncoderClkPin   = CONFIG_BTCTRL_ENCODER_CLK_GPIO;
constexpr int           kEncoderDtPin    = CONFIG_BTCTRL_ENCODER_DT_GPIO;
constexpr int           kEncoderSwPin    = CONFIG_BTCTRL_ENCODER_SW_GPIO;
constexpr int           kStatusLedPin    = CONFIG_BTCTRL_STATUS_LED_GPIO;
#ifdef CONFIG_BTCTRL_STATUS_LED_ACTIVE_LOW
constexpr bool          kStatusLedActiveLow = true;
#else
constexpr bool          kStatusLedActiveLow = false;
#endif
constexpr const char   *kDeviceName      = CONFIG_BTCTRL_DEVICE_NAME;
constexpr int           kBatteryAdcPin   = CONFIG_BTCTRL_BATTERY_ADC_GPIO;
constexpr int           kBatteryMvEmpty  = CONFIG_BTCTRL_BATTERY_MV_EMPTY;
constexpr int           kBatteryMvFull   = CONFIG_BTCTRL_BATTERY_MV_FULL;
constexpr std::uint32_t kSleepTimeoutMs  =
    static_cast<std::uint32_t>(CONFIG_BTCTRL_SLEEP_TIMEOUT_S) * 1000u;
#else
constexpr const char *kDeviceName = "Blusys Controller";
#endif

// ── Capability ───────────────────────────────────────────────────────────────
blusys::ble_hid_device_capability hid_device{{
    .device_name         = kDeviceName,
    .vendor_id           = 0x303A,  // Espressif — reasonable default for dev boards.
    .product_id          = 0x8001,
    .version             = 0x0100,
    .initial_battery_pct = 100,
}};

blusys::capability_list_storage capabilities{&hid_device};

// ── Dispatch plumbing ────────────────────────────────────────────────────────
blusys::app_ctx *s_ctx = nullptr;

#ifdef ESP_PLATFORM
std::uint32_t         s_last_activity_ms = 0;
std::atomic<bool>     s_activity_pending{false};
#endif

void dispatch_button(bluetooth_controller::intent btn)
{
#ifdef ESP_PLATFORM
    s_activity_pending.store(true, std::memory_order_relaxed);
#endif
    if (s_ctx == nullptr) {
        return;
    }
    bluetooth_controller::action act{};
    act.tag = bluetooth_controller::action_tag::button;
    act.btn = btn;
    s_ctx->dispatch(act);
}

// ── Simulator (host build + optional device simulation) ──────────────────────
// Fires 3× CW then 3× CCW once a client connects, to exercise the reducer
// without needing a physical encoder. Updated to use the new single-event
// turn_cw / turn_ccw intents.
struct click_simulator {
    static constexpr int           kClicksPerPhase = 3;
    static constexpr std::uint32_t kInterClickGapMs = 400;
    static constexpr std::uint32_t kPhaseGapMs      = 600;

    enum class phase : std::uint8_t { idle, cw, phase_gap, ccw, done };

    phase         phase_          = phase::idle;
    int           clicks_emitted_ = 0;
    std::uint32_t next_click_ms_  = 0;
    bool          was_connected_  = false;

    void arm(std::uint32_t now_ms)
    {
        phase_          = phase::cw;
        clicks_emitted_ = 0;
        next_click_ms_  = now_ms + kInterClickGapMs;
        BLUSYS_LOGI(kTag, "simulator ARMED — 3x CW (Vol+) then 3x CCW (Vol-)");
    }

    void reset()
    {
        phase_          = phase::idle;
        clicks_emitted_ = 0;
        next_click_ms_  = 0;
    }

    void tick(std::uint32_t now_ms, bool connected)
    {
        if (connected && !was_connected_)      arm(now_ms);
        else if (!connected && was_connected_) reset();
        was_connected_ = connected;

        if (!connected || phase_ == phase::idle || phase_ == phase::done) {
            return;
        }
        if (static_cast<std::int32_t>(now_ms - next_click_ms_) < 0) {
            return;
        }

        switch (phase_) {
        case phase::cw:
            dispatch_button(bluetooth_controller::intent::turn_cw);
            ++clicks_emitted_;
            next_click_ms_ = now_ms + kInterClickGapMs;
            if (clicks_emitted_ >= kClicksPerPhase) {
                phase_          = phase::phase_gap;
                clicks_emitted_ = 0;
                next_click_ms_  = now_ms + kPhaseGapMs;
                BLUSYS_LOGI(kTag, "simulator — CW done, pausing %u ms",
                            static_cast<unsigned>(kPhaseGapMs));
            }
            break;
        case phase::phase_gap:
            phase_         = phase::ccw;
            next_click_ms_ = now_ms;
            break;
        case phase::ccw:
            dispatch_button(bluetooth_controller::intent::turn_ccw);
            ++clicks_emitted_;
            next_click_ms_ = now_ms + kInterClickGapMs;
            if (clicks_emitted_ >= kClicksPerPhase) {
                phase_ = phase::done;
                BLUSYS_LOGI(kTag, "simulator — sequence complete (3x CW + 3x CCW)");
            }
            break;
        default:
            break;
        }
    }
};

click_simulator s_simulator{};

// ── HID send adapter ─────────────────────────────────────────────────────────
const char *usage_name(std::uint16_t usage)
{
    switch (usage) {
    case 0x00E9: return "VolumeUp";
    case 0x00EA: return "VolumeDown";
    case 0x00E2: return "Mute";
    case 0x00CD: return "PlayPause";
    case 0x00B5: return "NextTrack";
    case 0x00B6: return "PrevTrack";
    case 0x006F: return "BrightnessUp";
    case 0x0070: return "BrightnessDown";
    default:     return "?";
    }
}

void hid_send_adapter(std::uint16_t usage, bool pressed)
{
    auto err = hid_device.send_consumer(usage, pressed);
    bool connected = hid_device.is_connected();
    BLUSYS_LOGI(kTag, "HID → %-14s (0x%04x) %s  connected=%d  rc=%d",
                usage_name(usage), usage, pressed ? "press  " : "release",
                connected ? 1 : 0, static_cast<int>(err));
}

// ── Device-only: LED, encoder, battery, deep sleep ───────────────────────────
#ifdef ESP_PLATFORM

void led_set(bool on)
{
    if (kStatusLedPin < 0) {
        return;
    }
    blusys_gpio_write(kStatusLedPin, kStatusLedActiveLow ? !on : on);
}

void status_led_init()
{
    if (kStatusLedPin < 0) {
        return;
    }
    blusys_gpio_reset(kStatusLedPin);
    blusys_gpio_set_output(kStatusLedPin);
    led_set(false);
}

// Encoder ─────────────────────────────────────────────────────────────────────
blusys_encoder_t *s_encoder = nullptr;

void on_encoder_event(blusys_encoder_t * /*encoder*/,
                      blusys_encoder_event_t event,
                      int /*position*/,
                      void * /*user_ctx*/)
{
    using I = bluetooth_controller::intent;
    switch (event) {
    case BLUSYS_ENCODER_EVENT_CW:         dispatch_button(I::turn_cw);           break;
    case BLUSYS_ENCODER_EVENT_CCW:        dispatch_button(I::turn_ccw);          break;
    case BLUSYS_ENCODER_EVENT_PRESS:      dispatch_button(I::button_press);      break;
    case BLUSYS_ENCODER_EVENT_RELEASE:    dispatch_button(I::button_release);    break;
    case BLUSYS_ENCODER_EVENT_LONG_PRESS: dispatch_button(I::button_long_press); break;
    default: break;
    }
}

blusys_err_t open_encoder()
{
    blusys_encoder_config_t cfg{};
    cfg.clk_pin       = kEncoderClkPin;
    cfg.dt_pin        = kEncoderDtPin;
    cfg.sw_pin        = kEncoderSwPin;
    cfg.long_press_ms = CONFIG_BTCTRL_ENCODER_LONG_PRESS_MS;
    blusys_err_t err = blusys_encoder_open(&cfg, &s_encoder);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "encoder_open failed: %d", static_cast<int>(err));
        return err;
    }
    return blusys_encoder_set_callback(s_encoder, on_encoder_event, nullptr);
}

// Battery ADC ─────────────────────────────────────────────────────────────────
blusys_adc_t *s_battery_adc = nullptr;

void battery_init()
{
    if (kBatteryAdcPin < 0) {
        return;
    }
    blusys_err_t err = blusys_adc_open(kBatteryAdcPin,
                                        BLUSYS_ADC_ATTEN_DB_12,
                                        &s_battery_adc);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGW(kTag,
                    "battery ADC open(pin=%d) failed: %d — battery reporting disabled",
                    kBatteryAdcPin, static_cast<int>(err));
        s_battery_adc = nullptr;
    }
}

void battery_sample()
{
    if (s_battery_adc == nullptr) {
        return;
    }
    int mv = 0;
    if (blusys_adc_read_mv(s_battery_adc, &mv) != BLUSYS_OK) {
        return;
    }
    int range = kBatteryMvFull - kBatteryMvEmpty;
    int pct   = (range > 0) ? (mv - kBatteryMvEmpty) * 100 / range : 0;
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    hid_device.set_battery(static_cast<std::uint8_t>(pct));
    BLUSYS_LOGI(kTag, "battery: %d mV → %d%%", mv, pct);
}

// Deep sleep ──────────────────────────────────────────────────────────────────
void enter_deep_sleep()
{
    // Build wake mask only from valid (non-negative) pins.
    std::uint64_t wake_mask = 0;
    if (kEncoderClkPin >= 0) wake_mask |= (1ULL << kEncoderClkPin);
    if (kEncoderSwPin  >= 0) wake_mask |= (1ULL << kEncoderSwPin);

    if (wake_mask == 0) {
        BLUSYS_LOGW(kTag, "deep sleep skipped — no valid wake pins configured");
        return;
    }

    BLUSYS_LOGI(kTag,
                "inactivity timeout — entering deep sleep "
                "(wake: CLK GPIO %d, SW GPIO %d)",
                kEncoderClkPin, kEncoderSwPin);
    esp_deep_sleep_enable_gpio_wakeup(wake_mask, ESP_GPIO_WAKEUP_GPIO_LOW);
    blusys_sleep_enter_deep();
}

// ── SSD1306 OLED display ─────────────────────────────────────────────────────
#ifdef CONFIG_BTCTRL_DISPLAY_ENABLED

blusys_lcd_t     *s_lcd          = nullptr;
blusys_display_t *s_ui           = nullptr;
lv_obj_t         *s_mode_label   = nullptr;
lv_obj_t         *s_vol_bar      = nullptr;
lv_obj_t         *s_info_label   = nullptr;
lv_obj_t         *s_status_label = nullptr;

struct DisplaySnapshot {
    bluetooth_controller::op_state  phase{bluetooth_controller::op_state::idle};
    bluetooth_controller::ctrl_mode mode{bluetooth_controller::ctrl_mode::volume};
    int                             volume_est{-1};  // -1 forces first update
    bool                            muted{false};
} s_display_snap{};

void display_init()
{
    blusys_lcd_config_t lcd_cfg{};
    lcd_cfg.driver         = BLUSYS_LCD_DRIVER_SSD1306;
    lcd_cfg.width          = 128;
    lcd_cfg.height         = 64;
    lcd_cfg.bits_per_pixel = 1;
    lcd_cfg.i2c.port       = 0;
    lcd_cfg.i2c.sda_pin    = CONFIG_BTCTRL_DISPLAY_I2C_SDA_GPIO;
    lcd_cfg.i2c.scl_pin    = CONFIG_BTCTRL_DISPLAY_I2C_SCL_GPIO;
    lcd_cfg.i2c.dev_addr   = 0x3C;
    lcd_cfg.i2c.rst_pin    = -1;
    lcd_cfg.i2c.freq_hz    = 400000;

    blusys_err_t err = blusys_lcd_open(&lcd_cfg, &s_lcd);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "display lcd_open failed: %d", static_cast<int>(err));
        return;
    }

    blusys_display_config_t ui_cfg{};
    ui_cfg.lcd        = s_lcd;
    ui_cfg.panel_kind = BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE;
    err = blusys_display_open(&ui_cfg, &s_ui);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "display_open failed: %d", static_cast<int>(err));
        blusys_lcd_close(s_lcd);
        s_lcd = nullptr;
        return;
    }

    blusys_display_lock(s_ui);

    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);

    // Mode name — top, large
    s_mode_label = lv_label_create(screen);
    lv_obj_set_style_text_font(s_mode_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_mode_label, lv_color_white(), 0);
    lv_obj_set_pos(s_mode_label, 0, 2);
    lv_obj_set_width(s_mode_label, 128);
    lv_obj_set_style_text_align(s_mode_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_mode_label, "VOLUME");

    // Volume bar — middle (shown only in volume mode)
    s_vol_bar = lv_bar_create(screen);
    lv_obj_set_size(s_vol_bar, 116, 8);
    lv_obj_set_pos(s_vol_bar, 6, 28);
    lv_bar_set_range(s_vol_bar, 0, 100);
    lv_bar_set_value(s_vol_bar, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_vol_bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_vol_bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_vol_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_vol_bar, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_vol_bar, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_vol_bar, 2, LV_PART_INDICATOR);

    // Info label — volume % / MUTED / mode subtitle
    s_info_label = lv_label_create(screen);
    lv_obj_set_style_text_font(s_info_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_info_label, lv_color_white(), 0);
    lv_obj_set_pos(s_info_label, 0, 38);
    lv_obj_set_width(s_info_label, 128);
    lv_obj_set_style_text_align(s_info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_info_label, "50%");

    // Status label — BLE state
    s_status_label = lv_label_create(screen);
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_status_label, lv_color_white(), 0);
    lv_obj_set_pos(s_status_label, 0, 50);
    lv_obj_set_width(s_status_label, 128);
    lv_obj_set_style_text_align(s_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_status_label, "Idle");

    lv_screen_load(screen);
    blusys_display_unlock(s_ui);

    BLUSYS_LOGI(kTag, "SSD1306 ready (SDA=%d SCL=%d)",
                CONFIG_BTCTRL_DISPLAY_I2C_SDA_GPIO,
                CONFIG_BTCTRL_DISPLAY_I2C_SCL_GPIO);
}

void display_update(const app_state &state)
{
    if (s_ui == nullptr) {
        return;
    }
    bool changed = (state.phase      != s_display_snap.phase)
                || (state.mode       != s_display_snap.mode)
                || (state.volume_est != s_display_snap.volume_est)
                || (state.muted      != s_display_snap.muted);
    if (!changed) {
        return;
    }
    s_display_snap.phase      = state.phase;
    s_display_snap.mode       = state.mode;
    s_display_snap.volume_est = state.volume_est;
    s_display_snap.muted      = state.muted;

    blusys_display_lock(s_ui);

    // Mode label
    switch (state.mode) {
    case ctrl_mode::volume:     lv_label_set_text(s_mode_label, "VOLUME");     break;
    case ctrl_mode::media:      lv_label_set_text(s_mode_label, "MEDIA");      break;
    case ctrl_mode::brightness: lv_label_set_text(s_mode_label, "BRIGHTNESS"); break;
    }

    // Volume bar + info label
    if (state.mode == ctrl_mode::volume) {
        lv_obj_remove_flag(s_vol_bar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(s_vol_bar, state.volume_est, LV_ANIM_OFF);
        if (state.muted) {
            lv_label_set_text(s_info_label, "MUTED");
        } else {
            lv_label_set_text_fmt(s_info_label, "%d%%", state.volume_est);
        }
    } else {
        lv_obj_add_flag(s_vol_bar, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_info_label,
            state.mode == ctrl_mode::media ? "Track Control" : "Brightness");
    }

    // Status label
    switch (state.phase) {
    case op_state::idle:        lv_label_set_text(s_status_label, "Idle");       break;
    case op_state::advertising: lv_label_set_text(s_status_label, "Pairing..."); break;
    case op_state::connected:   lv_label_set_text(s_status_label, "Connected");  break;
    }

    blusys_display_unlock(s_ui);
}

#endif  // CONFIG_BTCTRL_DISPLAY_ENABLED

// on_init / on_tick ───────────────────────────────────────────────────────────
void on_init_device(blusys::app_ctx &ctx, blusys::app_services & /*svc*/,
                    app_state & /*state*/)
{
    s_ctx = &ctx;
    bluetooth_controller::set_hid_send(hid_send_adapter);
    bluetooth_controller::set_led_setter(led_set);
    s_activity_pending.store(true, std::memory_order_relaxed);  // first tick stamps s_last_activity_ms
    status_led_init();
    battery_init();
    open_encoder();
    BLUSYS_LOGI(kTag,
                "bluetooth controller up — "
                "encoder: clk=%d dt=%d sw=%d  long_press=%dms  "
                "battery_pin=%d  sleep_timeout=%us  led=%d",
                kEncoderClkPin, kEncoderDtPin, kEncoderSwPin,
                CONFIG_BTCTRL_ENCODER_LONG_PRESS_MS,
                kBatteryAdcPin,
                static_cast<unsigned>(CONFIG_BTCTRL_SLEEP_TIMEOUT_S),
                kStatusLedPin);
#ifdef CONFIG_BTCTRL_SIMULATE_ON_CONNECT
    BLUSYS_LOGI(kTag, "auto-simulation enabled — 3x CW then 3x CCW on each connect");
#endif
#ifdef CONFIG_BTCTRL_DISPLAY_ENABLED
    display_init();
#endif
}

void on_tick_device(blusys::app_ctx & /*ctx*/, blusys::app_services & /*svc*/,
                    app_state &state, std::uint32_t now_ms)
{
#ifdef CONFIG_BTCTRL_SIMULATE_ON_CONNECT
    s_simulator.tick(now_ms, hid_device.is_ready());
#endif

    // Battery: sample every 30 s
    static std::uint32_t s_last_battery_ms = 0;
    if (s_battery_adc != nullptr && now_ms - s_last_battery_ms >= 30'000u) {
        s_last_battery_ms = now_ms;
        battery_sample();
    }

    // Consume the activity flag written by dispatch_button (encoder callback context).
    // Stamp using this task's now_ms so both sides of the comparison use the same clock.
    if (s_activity_pending.exchange(false, std::memory_order_relaxed)) {
        s_last_activity_ms = now_ms;
    }

    // Deep sleep: enter after configured inactivity period (0 = disabled)
    if (kSleepTimeoutMs > 0u && now_ms - s_last_activity_ms >= kSleepTimeoutMs) {
        enter_deep_sleep();
    }

#ifdef CONFIG_BTCTRL_DISPLAY_ENABLED
    display_update(state);
#endif
}

#else  // host build ─────────────────────────────────────────────────────────

void led_set(bool on)
{
    BLUSYS_LOGI(kTag, "[led] %s", on ? "solid (connected)" : "blink (advertising)");
}

void on_init_host(blusys::app_ctx &ctx, blusys::app_services & /*svc*/,
                  app_state & /*state*/)
{
    s_ctx = &ctx;
    bluetooth_controller::set_hid_send(hid_send_adapter);
    bluetooth_controller::set_led_setter(led_set);
    BLUSYS_LOGI(kTag, "bluetooth controller host stub — simulating encoder turns");
}

void on_tick_host(blusys::app_ctx &ctx, blusys::app_services & /*svc*/,
                  app_state &state, std::uint32_t now_ms)
{
    s_ctx = &ctx;
    s_simulator.tick(now_ms, state.ble_connected);
}

#endif  // ESP_PLATFORM

// ── App spec ─────────────────────────────────────────────────────────────────
const blusys::app_spec<app_state, action> spec{
    .initial_state = {},
    .update        = update,
#ifdef ESP_PLATFORM
    .on_init       = on_init_device,
    .on_tick       = on_tick_device,
#else
    .on_init       = on_init_host,
    .on_tick       = on_tick_host,
#endif
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .tick_period_ms = 50,
    .capabilities  = &capabilities,
};

}  // namespace

}  // namespace bluetooth_controller::system

BLUSYS_APP_MAIN_HEADLESS(bluetooth_controller::system::spec)
