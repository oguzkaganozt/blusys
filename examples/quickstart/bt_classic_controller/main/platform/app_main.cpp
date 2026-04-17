// BT Classic controller platform layer.
//
// Three concurrent Bluetooth profiles on the original ESP32:
//   1. Classic BT HID Device     — sends consumer-control reports to the host
//   2. A2DP Sink (stub)          — lets phones auto-connect AVRCP; audio discarded
//   3. AVRCP Controller          — reads now-playing track title + artist
//
// Display (SSD1306 128×64 OLED via I2C):
//   Volume mode   : mode label + volume bar + "XX%" or "MUTED"
//   Media mode    : mode label + track title + artist (from AVRCP)
//   Brightness mode: mode label + "Brightness"
//
// Deep sleep:
//   ESP32 ext1 wakeup on GPIO32 (CLK) — ALL_LOW, single-pin mask.
//   GPIO23 (SW button) is NOT an RTC GPIO on ESP32 and cannot be in the mask.

#include "core/app_logic.hpp"
#include "blusys/blusys.hpp"

#include "blusys_examples/bt_controller_hw.hpp"

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_hidd_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#endif

#if defined(ESP_PLATFORM) && defined(CONFIG_BTCC_DISPLAY_ENABLED)
#include "lvgl.h"
#endif

#include <atomic>
#include <cstdint>
#include <cstring>

namespace bt_classic_controller::system {

namespace {

constexpr const char *kTag = "btcc";

// ── Platform constants ────────────────────────────────────────────────────────
#ifdef ESP_PLATFORM
constexpr int           kEncoderClkPin     = CONFIG_BTCC_ENCODER_CLK_GPIO;
constexpr int           kEncoderDtPin      = CONFIG_BTCC_ENCODER_DT_GPIO;
constexpr int           kEncoderSwPin      = CONFIG_BTCC_ENCODER_SW_GPIO;
constexpr int           kStatusLedPin      = CONFIG_BTCC_STATUS_LED_GPIO;
#ifdef CONFIG_BTCC_STATUS_LED_ACTIVE_LOW
constexpr bool          kStatusLedActiveLow = true;
#else
constexpr bool          kStatusLedActiveLow = false;
#endif
constexpr const char   *kDeviceName        = CONFIG_BTCC_DEVICE_NAME;
constexpr int           kBatteryAdcPin     = CONFIG_BTCC_BATTERY_ADC_GPIO;
constexpr int           kBatteryMvEmpty    = CONFIG_BTCC_BATTERY_MV_EMPTY;
constexpr int           kBatteryMvFull     = CONFIG_BTCC_BATTERY_MV_FULL;
constexpr std::uint32_t kSleepTimeoutMs    =
    static_cast<std::uint32_t>(CONFIG_BTCC_SLEEP_TIMEOUT_S) * 1000u;
#else
constexpr const char *kDeviceName = "Oxa BT Controller";
#endif

// ── Dispatch plumbing ─────────────────────────────────────────────────────────
blusys::app_ctx *s_ctx = nullptr;

#ifdef ESP_PLATFORM

// ── Activity / deep-sleep tracking ───────────────────────────────────────────
std::uint32_t     s_last_activity_ms = 0;
std::atomic<bool> s_activity_pending{false};

// ── HID connection flags (written from BT task, consumed in on_tick) ─────────
std::atomic<bool> s_hid_connected{false};
std::atomic<bool> s_hid_just_connected{false};
std::atomic<bool> s_hid_just_disconnected{false};

// ── AVRCP flags ───────────────────────────────────────────────────────────────
std::atomic<bool> s_avrc_just_connected{false};
std::atomic<bool> s_avrc_just_disconnected{false};

// ── Track metadata accumulation ───────────────────────────────────────────────
// The AVRCP metadata callback fires once per attribute (title, then artist).
// We accumulate them under a spinlock and signal on_tick only when both arrive.
static portMUX_TYPE s_track_mux        = portMUX_INITIALIZER_UNLOCKED;
static char         s_pend_title[128]  = {};
static char         s_pend_artist[128] = {};
static bool         s_pend_title_valid  = false;
static bool         s_pend_artist_valid = false;
std::atomic<bool>   s_track_pending{false};

// ── AVRCP transaction-label allocator (cycles 0–15 per AVRCP spec) ───────────
static std::uint8_t s_avrc_tl = 0;
static std::uint8_t alloc_tl()
{
    return (s_avrc_tl = (s_avrc_tl + 1u) & 0x0Fu);
}

// ── Remote notification capability mask ──────────────────────────────────────
static esp_avrc_rn_evt_cap_mask_t s_peer_rn_cap = {};

// ── HID Consumer Control report descriptor (report ID 1, 16-bit usage array) ─
// One 16-bit consumer usage per report: non-zero = key pressed, 0 = key released.
static const std::uint8_t kHidDescriptor[] = {
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,  //   Usage Maximum (1023)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x00,        //   Input (Data, Array, Absolute)
    0xC0,              // End Collection
};

#endif  // ESP_PLATFORM

// ── HID send adapter ──────────────────────────────────────────────────────────
void hid_send_adapter(std::uint16_t usage, bool pressed)
{
#ifdef ESP_PLATFORM
    if (!s_hid_connected.load(std::memory_order_relaxed)) {
        return;
    }
    std::uint8_t report[2];
    if (pressed) {
        report[0] = static_cast<std::uint8_t>(usage & 0xFFu);
        report[1] = static_cast<std::uint8_t>((usage >> 8u) & 0xFFu);
    } else {
        report[0] = 0;
        report[1] = 0;
    }
    esp_err_t err = esp_bt_hid_device_send_report(
        ESP_HIDD_REPORT_TYPE_INTRDATA, 0x01, 2, report);
    BLUSYS_LOGI(kTag, "HID → 0x%04x %s (rc=%d)",
                usage, pressed ? "press" : "release",
                static_cast<int>(err));
#else
    BLUSYS_LOGI(kTag, "[hid stub] 0x%04x %s", usage, pressed ? "press" : "release");
#endif
}

// ── Encoder callback (encoder ISR/task → dispatch_button → activity flag) ────
void dispatch_button(bt_classic_controller::intent btn)
{
#ifdef ESP_PLATFORM
    s_activity_pending.store(true, std::memory_order_relaxed);
#endif
    if (s_ctx == nullptr) {
        return;
    }
    bt_classic_controller::action act{};
    act.tag = bt_classic_controller::action_tag::button;
    act.btn = btn;
    s_ctx->dispatch(act);
}

#ifdef ESP_PLATFORM

// ── Peripherals (LED, encoder, battery) ──────────────────────────────────────
blusys_examples::status_led      s_led;
blusys_examples::battery_monitor s_battery;
blusys_examples::encoder_input   s_encoder;

void led_set(bool on) { s_led.set(on); }

void on_encoder_event(blusys_examples::encoder_event ev)
{
    dispatch_button(
        blusys_examples::map_encoder_intent<bt_classic_controller::intent>(ev));
}

// ── Deep sleep ────────────────────────────────────────────────────────────────
void enter_deep_sleep()
{
    // On ESP32, ext1 wakeup requires RTC-capable GPIOs: 0,2,4,12-15,25-27,32-39.
    // GPIO23 (SW button) is NOT in that list — use CLK (GPIO32) only.
    if (kEncoderClkPin < 0) {
        BLUSYS_LOGW(kTag, "deep sleep skipped — no valid RTC wake pin configured");
        return;
    }
    std::uint64_t wake_mask = (1ULL << kEncoderClkPin);

    BLUSYS_LOGI(kTag,
                "inactivity timeout — entering deep sleep (wake: GPIO%d low)",
                kEncoderClkPin);

    // ALL_LOW on a single-pin mask: wakes when that one pin goes low.
    esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ALL_LOW);
    blusys_sleep_enter_deep();
}

// ── Bluetooth callbacks ───────────────────────────────────────────────────────

void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            BLUSYS_LOGI(kTag, "GAP auth success: %s", param->auth_cmpl.device_name);
        } else {
            BLUSYS_LOGE(kTag, "GAP auth failed: %d", param->auth_cmpl.stat);
        }
        break;
    case ESP_BT_GAP_CFM_REQ_EVT:
        // SSP — auto-confirm (headless device has no display for passkey).
        BLUSYS_LOGI(kTag, "GAP SSP confirm: %lu",
                    static_cast<unsigned long>(param->cfm_req.num_val));
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_PIN_REQ_EVT:
        // Legacy PIN pairing — respond with "0000".
        BLUSYS_LOGI(kTag, "GAP PIN request");
        if (param->pin_req.min_16_digit) {
            esp_bt_pin_code_t pin = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin);
        } else {
            esp_bt_pin_code_t pin = {'0', '0', '0', '0'};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin);
        }
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        BLUSYS_LOGI(kTag, "GAP SSP passkey: %06lu",
                    static_cast<unsigned long>(param->key_notif.passkey));
        break;
    default:
        break;
    }
}

void hidd_cb(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    static esp_hidd_app_param_t s_app_param;
    static esp_hidd_qos_param_t s_qos;

    switch (event) {
    case ESP_HIDD_INIT_EVT:
        if (param->init.status == ESP_HIDD_SUCCESS) {
            BLUSYS_LOGI(kTag, "HID init OK — registering app");
            s_app_param.name          = "BT Classic Controller";
            s_app_param.description   = "Consumer Control HID";
            s_app_param.provider      = "Oxa";
            s_app_param.subclass      = 0x00;  // generic HID
            s_app_param.desc_list     = const_cast<std::uint8_t *>(kHidDescriptor);
            s_app_param.desc_list_len = static_cast<int>(sizeof(kHidDescriptor));
            std::memset(&s_qos, 0, sizeof(s_qos));
            esp_bt_hid_device_register_app(&s_app_param, &s_qos, &s_qos);
        }
        break;
    case ESP_HIDD_REGISTER_APP_EVT:
        if (param->register_app.status == ESP_HIDD_SUCCESS) {
            BLUSYS_LOGI(kTag, "HID app registered — connectable+discoverable");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,
                                     ESP_BT_GENERAL_DISCOVERABLE);
            // If a previously bonded host exists, attempt reconnection.
            if (param->register_app.in_use) {
                BLUSYS_LOGI(kTag, "HID: reconnecting to bonded host");
                esp_bt_hid_device_connect(param->register_app.bd_addr);
            }
        }
        break;
    case ESP_HIDD_OPEN_EVT:
        if (param->open.status == ESP_HIDD_SUCCESS &&
            param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTED) {
            BLUSYS_LOGI(kTag, "HID open — host connected");
            s_hid_connected.store(true, std::memory_order_relaxed);
            s_hid_just_connected.store(true, std::memory_order_relaxed);
            s_activity_pending.store(true, std::memory_order_relaxed);
            led_set(true);
            // Stop advertising once connected.
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE,
                                     ESP_BT_NON_DISCOVERABLE);
        }
        break;
    case ESP_HIDD_CLOSE_EVT:
        if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED) {
            BLUSYS_LOGI(kTag, "HID close — host disconnected");
            s_hid_connected.store(false, std::memory_order_relaxed);
            s_hid_just_disconnected.store(true, std::memory_order_relaxed);
            led_set(false);
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,
                                     ESP_BT_GENERAL_DISCOVERABLE);
        }
        break;
    default:
        break;
    }
}

// A2DP Sink stub — initialised so phones auto-connect AVRCP alongside A2DP.
// Audio data is intentionally discarded; we have no speaker hardware.
void a2dp_data_cb(const std::uint8_t * /*data*/, std::uint32_t /*len*/)
{
    // Discard: no audio output needed — we only want AVRCP metadata.
}

void a2dp_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        BLUSYS_LOGI(kTag, "A2DP state: %d", static_cast<int>(param->conn_stat.state));
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        BLUSYS_LOGI(kTag, "A2DP audio: %d",
                    static_cast<int>(param->audio_stat.state));
        break;
    default:
        break;
    }
}

void avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event) {

    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
        if (param->conn_stat.connected) {
            BLUSYS_LOGI(kTag, "AVRC CT connected");
            s_avrc_just_connected.store(true, std::memory_order_relaxed);

            // Clear any stale pending metadata under spinlock.
            taskENTER_CRITICAL(&s_track_mux);
            s_pend_title_valid  = false;
            s_pend_artist_valid = false;
            taskEXIT_CRITICAL(&s_track_mux);

            // Ask peer what AVRCP change notifications it supports.
            esp_avrc_ct_send_get_rn_capabilities_cmd(alloc_tl());
        } else {
            BLUSYS_LOGI(kTag, "AVRC CT disconnected");
            s_avrc_just_disconnected.store(true, std::memory_order_relaxed);
            taskENTER_CRITICAL(&s_track_mux);
            s_pend_title_valid  = false;
            s_pend_artist_valid = false;
            taskEXIT_CRITICAL(&s_track_mux);
        }
        break;

    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
        // Store peer capabilities mask, subscribe to TRACK_CHANGE and
        // PLAY_STATUS_CHANGE, then immediately request the current track.
        //
        // PLAY_STATUS_CHANGE is critical: when a track is already loaded but
        // paused at connect time the initial metadata query returns empty
        // strings.  When the user hits Play, TRACK_CHANGE doesn't fire
        // (same track), but PLAY_STATUS_CHANGE does — we use it to re-query.
        s_peer_rn_cap = param->get_rn_caps_rsp.evt_set;
        BLUSYS_LOGI(kTag, "AVRC RN caps: 0x%x", s_peer_rn_cap.bits);

        if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST,
                                               &s_peer_rn_cap,
                                               ESP_AVRC_RN_TRACK_CHANGE)) {
            esp_avrc_ct_send_register_notification_cmd(
                alloc_tl(), ESP_AVRC_RN_TRACK_CHANGE, 0);
        }
        if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST,
                                               &s_peer_rn_cap,
                                               ESP_AVRC_RN_PLAY_STATUS_CHANGE)) {
            esp_avrc_ct_send_register_notification_cmd(
                alloc_tl(), ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
        }
        esp_avrc_ct_send_metadata_cmd(alloc_tl(),
            ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST);
        break;

    case ESP_AVRC_CT_METADATA_RSP_EVT: {
        // Metadata arrives one attribute at a time — accumulate title and artist
        // under spinlock, then signal on_tick when both have been received.
        // IMPORTANT: attr_text is only valid inside this callback; copy immediately.
        bool both_ready = false;
        int  copy_len   = param->meta_rsp.attr_length;

        taskENTER_CRITICAL(&s_track_mux);
        if (param->meta_rsp.attr_id == ESP_AVRC_MD_ATTR_TITLE) {
            if (copy_len > 127) copy_len = 127;
            std::memcpy(s_pend_title, param->meta_rsp.attr_text,
                        static_cast<std::size_t>(copy_len));
            s_pend_title[copy_len] = '\0';
            s_pend_title_valid     = true;
        } else if (param->meta_rsp.attr_id == ESP_AVRC_MD_ATTR_ARTIST) {
            if (copy_len > 127) copy_len = 127;
            std::memcpy(s_pend_artist, param->meta_rsp.attr_text,
                        static_cast<std::size_t>(copy_len));
            s_pend_artist[copy_len] = '\0';
            s_pend_artist_valid     = true;
        }
        if (s_pend_title_valid && s_pend_artist_valid) {
            both_ready          = true;
            s_pend_title_valid  = false;
            s_pend_artist_valid = false;
        }
        taskEXIT_CRITICAL(&s_track_mux);

        if (both_ready) {
            s_track_pending.store(true, std::memory_order_relaxed);
        }
        break;
    }

    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        if (param->change_ntf.event_id == ESP_AVRC_RN_TRACK_CHANGE) {
            BLUSYS_LOGI(kTag, "AVRC track changed — re-registering");

            // Clear stale pending data from the previous track.
            taskENTER_CRITICAL(&s_track_mux);
            s_pend_title_valid  = false;
            s_pend_artist_valid = false;
            taskEXIT_CRITICAL(&s_track_mux);

            // AVRCP spec: notification must be re-registered after each CHANGED event.
            if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST,
                                                   &s_peer_rn_cap,
                                                   ESP_AVRC_RN_TRACK_CHANGE)) {
                esp_avrc_ct_send_register_notification_cmd(
                    alloc_tl(), ESP_AVRC_RN_TRACK_CHANGE, 0);
            }
            esp_avrc_ct_send_metadata_cmd(alloc_tl(),
                ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST);

        } else if (param->change_ntf.event_id == ESP_AVRC_RN_PLAY_STATUS_CHANGE) {
            BLUSYS_LOGI(kTag, "AVRC play status changed — re-registering, fetching metadata");

            // Re-register so we get the next status change.
            if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST,
                                                   &s_peer_rn_cap,
                                                   ESP_AVRC_RN_PLAY_STATUS_CHANGE)) {
                esp_avrc_ct_send_register_notification_cmd(
                    alloc_tl(), ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
            }
            // Re-query metadata: if the track was already loaded but paused at
            // connect time (causing empty strings), this refreshes on resume.
            taskENTER_CRITICAL(&s_track_mux);
            s_pend_title_valid  = false;
            s_pend_artist_valid = false;
            taskEXIT_CRITICAL(&s_track_mux);
            esp_avrc_ct_send_metadata_cmd(alloc_tl(),
                ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST);
        }
        break;

    default:
        break;
    }
}

// ── SSD1306 OLED display ──────────────────────────────────────────────────────
#ifdef CONFIG_BTCC_DISPLAY_ENABLED

blusys_lcd_t     *s_lcd          = nullptr;
blusys_display_t *s_ui           = nullptr;
lv_obj_t         *s_mode_label   = nullptr;
lv_obj_t         *s_vol_bar      = nullptr;
lv_obj_t         *s_info_label   = nullptr;  // vol%/MUTED or track title
lv_obj_t         *s_track_label  = nullptr;  // artist (media mode only)
lv_obj_t         *s_status_label = nullptr;

struct DisplaySnapshot {
    bt_classic_controller::op_state  phase{bt_classic_controller::op_state::idle};
    bt_classic_controller::ctrl_mode mode{bt_classic_controller::ctrl_mode::volume};
    int  volume_est{-1};
    bool muted{false};
    bool avrc_connected{false};
    char title[128]{};
    char artist[128]{};
} s_display_snap{};

void display_init()
{
    blusys_lcd_config_t lcd_cfg{};
    lcd_cfg.driver         = BLUSYS_LCD_DRIVER_SSD1306;
    lcd_cfg.width          = 128;
    lcd_cfg.height         = 64;
    lcd_cfg.bits_per_pixel = 1;
    lcd_cfg.i2c.port       = 0;
    lcd_cfg.i2c.sda_pin    = CONFIG_BTCC_DISPLAY_I2C_SDA_GPIO;
    lcd_cfg.i2c.scl_pin    = CONFIG_BTCC_DISPLAY_I2C_SCL_GPIO;
    lcd_cfg.i2c.dev_addr   = 0x3C;
    lcd_cfg.i2c.rst_pin    = -1;
    lcd_cfg.i2c.freq_hz    = 400000;

    blusys_err_t err = blusys_lcd_open(&lcd_cfg, &s_lcd);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "lcd_open failed: %d", static_cast<int>(err));
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

    // Mode name — top, large font.
    s_mode_label = lv_label_create(screen);
    lv_obj_set_style_text_font(s_mode_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_mode_label, lv_color_white(), 0);
    lv_obj_set_pos(s_mode_label, 0, 2);
    lv_obj_set_width(s_mode_label, 128);
    lv_obj_set_style_text_align(s_mode_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_mode_label, "VOLUME");

    // Volume bar — visible in volume mode only.
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

    // Info line — "50%" / "MUTED" / track title.
    s_info_label = lv_label_create(screen);
    lv_obj_set_style_text_font(s_info_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_info_label, lv_color_white(), 0);
    lv_obj_set_pos(s_info_label, 0, 38);
    lv_obj_set_width(s_info_label, 128);
    lv_obj_set_style_text_align(s_info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(s_info_label, LV_LABEL_LONG_DOT);
    lv_label_set_text(s_info_label, "50%");

    // Artist line — visible in media mode only.
    s_track_label = lv_label_create(screen);
    lv_obj_set_style_text_font(s_track_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_track_label, lv_color_white(), 0);
    lv_obj_set_pos(s_track_label, 0, 38);
    lv_obj_set_width(s_track_label, 128);
    lv_obj_set_style_text_align(s_track_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(s_track_label, LV_LABEL_LONG_DOT);
    lv_obj_add_flag(s_track_label, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s_track_label, "");

    // BT/AVRC status — bottom.
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
                CONFIG_BTCC_DISPLAY_I2C_SDA_GPIO,
                CONFIG_BTCC_DISPLAY_I2C_SCL_GPIO);
}

void display_update(const bt_classic_controller::app_state &state)
{
    if (s_ui == nullptr) {
        return;
    }

    using ctrl_mode = bt_classic_controller::ctrl_mode;
    using op_state  = bt_classic_controller::op_state;

    const bool changed =
        (state.phase        != s_display_snap.phase)
     || (state.mode         != s_display_snap.mode)
     || (state.volume_est   != s_display_snap.volume_est)
     || (state.muted        != s_display_snap.muted)
     || (state.avrc_connected != s_display_snap.avrc_connected)
     || (std::strncmp(state.track.title,  s_display_snap.title,  127) != 0)
     || (std::strncmp(state.track.artist, s_display_snap.artist, 127) != 0);

    if (!changed) {
        return;
    }

    s_display_snap.phase          = state.phase;
    s_display_snap.mode           = state.mode;
    s_display_snap.volume_est     = state.volume_est;
    s_display_snap.muted          = state.muted;
    s_display_snap.avrc_connected = state.avrc_connected;
    std::strncpy(s_display_snap.title,  state.track.title,  127);
    std::strncpy(s_display_snap.artist, state.track.artist, 127);

    blusys_display_lock(s_ui);

    // ── Mode label ────────────────────────────────────────────────────────────
    switch (state.mode) {
    case ctrl_mode::volume:     lv_label_set_text(s_mode_label, "VOLUME");     break;
    case ctrl_mode::media:      lv_label_set_text(s_mode_label, "MEDIA");      break;
    case ctrl_mode::brightness: lv_label_set_text(s_mode_label, "BRIGHTNESS"); break;
    }

    // ── Content area (y=26..50) ───────────────────────────────────────────────
    if (state.mode == ctrl_mode::volume) {
        lv_obj_remove_flag(s_vol_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_track_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(s_info_label, 0, 38);
        lv_bar_set_value(s_vol_bar, state.volume_est, LV_ANIM_OFF);
        lv_label_set_text(s_info_label,
            state.muted ? "MUTED"
                        : (lv_label_get_text(s_info_label),  // force redraw
                           (char[]){'\0'}) );                 // placeholder
        if (!state.muted) {
            lv_label_set_text_fmt(s_info_label, "%d%%", state.volume_est);
        }

    } else if (state.mode == ctrl_mode::media) {
        lv_obj_add_flag(s_vol_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_track_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(s_info_label, 0, 26);

        if (state.track.title[0] != '\0') {
            lv_label_set_text(s_info_label,  state.track.title);
            lv_label_set_text(s_track_label, state.track.artist);
        } else {
            lv_label_set_text(s_info_label, "No Track Info");
            lv_label_set_text(s_track_label,
                state.avrc_connected ? "Waiting..." : "No AVRCP");
        }

    } else {
        // Brightness
        lv_obj_add_flag(s_vol_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_track_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(s_info_label, 0, 38);
        lv_label_set_text(s_info_label, "Brightness");
    }

    // ── Status line ───────────────────────────────────────────────────────────
    switch (state.phase) {
    case op_state::idle:         lv_label_set_text(s_status_label, "Idle");       break;
    case op_state::discoverable: lv_label_set_text(s_status_label, "Pairing..."); break;
    case op_state::connected:
        lv_label_set_text(s_status_label,
            state.avrc_connected ? "Connected" : "Connected (no AVRC)");
        break;
    }

    blusys_display_unlock(s_ui);
}

#endif  // CONFIG_BTCC_DISPLAY_ENABLED

// ── on_init_device ────────────────────────────────────────────────────────────
void on_init_device(blusys::app_ctx &ctx, blusys::app_fx & /*fx*/,
                    bt_classic_controller::app_state & /*state*/)
{
    s_ctx = &ctx;
    bt_classic_controller::set_hid_send(hid_send_adapter);

    // ── 1. NVS (required by Bluedroid for bond storage) ───────────────────────
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // ── 2. Release BLE memory — we only use Classic BT ───────────────────────
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    // ── 3–4. BT controller ───────────────────────────────────────────────────
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    // ── 5–6. Bluedroid ───────────────────────────────────────────────────────
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // ── 7. GAP ───────────────────────────────────────────────────────────────
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_cb));
    ESP_ERROR_CHECK(esp_bt_gap_set_device_name(kDeviceName));

    // COD: peripheral major class + audio service bit (for A2DP discoverability).
    esp_bt_cod_t cod = {};
    cod.major   = ESP_BT_COD_MAJOR_DEV_PERIPHERAL;
    cod.minor   = 0x03;  // remote control
    cod.service = ESP_BT_COD_SRVC_AUDIO;
    esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_ALL);

    // IO capability: NoInputNoOutput — auto-confirms SSP on headless devices.
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap));

    // ── 8. AVRCP Controller (must be before A2DP per IDF note) ───────────────
    ESP_ERROR_CHECK(esp_avrc_ct_register_callback(avrc_ct_cb));
    ESP_ERROR_CHECK(esp_avrc_ct_init());

    // ── 9. A2DP Sink stub ─────────────────────────────────────────────────────
    ESP_ERROR_CHECK(esp_a2d_register_callback(a2dp_cb));
    ESP_ERROR_CHECK(esp_a2d_sink_init());
    ESP_ERROR_CHECK(esp_a2d_sink_register_data_callback(a2dp_data_cb));

    // ── 10. HID Device ───────────────────────────────────────────────────────
    // Registration completes asynchronously via hidd_cb:
    //   INIT_EVT → register_app() → REGISTER_APP_EVT → set_scan_mode()
    ESP_ERROR_CHECK(esp_bt_hid_device_register_callback(hidd_cb));
    ESP_ERROR_CHECK(esp_bt_hid_device_init());

    // ── 11. Peripherals ──────────────────────────────────────────────────────
    s_led.init({.pin = kStatusLedPin, .active_low = kStatusLedActiveLow});
    s_battery.init({.adc_pin  = kBatteryAdcPin,
                    .mv_empty = kBatteryMvEmpty,
                    .mv_full  = kBatteryMvFull});
    s_encoder.open({.clk_pin       = kEncoderClkPin,
                    .dt_pin        = kEncoderDtPin,
                    .sw_pin        = kEncoderSwPin,
                    .long_press_ms = CONFIG_BTCC_ENCODER_LONG_PRESS_MS},
                   on_encoder_event);

    // Stamp first activity so the 120s sleep clock starts now.
    s_activity_pending.store(true, std::memory_order_relaxed);

    BLUSYS_LOGI(kTag,
                "bt_classic_controller up — "
                "enc: clk=%d dt=%d sw=%d  long_press=%dms  "
                "sleep=%us  battery_pin=%d  led=%d",
                kEncoderClkPin, kEncoderDtPin, kEncoderSwPin,
                CONFIG_BTCC_ENCODER_LONG_PRESS_MS,
                static_cast<unsigned>(CONFIG_BTCC_SLEEP_TIMEOUT_S),
                kBatteryAdcPin, kStatusLedPin);

#ifdef CONFIG_BTCC_DISPLAY_ENABLED
    display_init();
#endif
}

// ── on_tick_device ────────────────────────────────────────────────────────────
void on_tick_device(blusys::app_ctx &ctx, blusys::app_fx & /*fx*/,
                    bt_classic_controller::app_state &state,
                    std::uint32_t now_ms)
{
    using AT = bt_classic_controller::action_tag;

    // ── Consume BT connection-state flags ─────────────────────────────────────
    if (s_hid_just_connected.exchange(false, std::memory_order_relaxed)) {
        bt_classic_controller::action act{};
        act.tag = AT::hid_connected;
        ctx.dispatch(act);
    }
    if (s_hid_just_disconnected.exchange(false, std::memory_order_relaxed)) {
        bt_classic_controller::action act{};
        act.tag = AT::hid_disconnected;
        ctx.dispatch(act);
    }
    if (s_avrc_just_connected.exchange(false, std::memory_order_relaxed)) {
        bt_classic_controller::action act{};
        act.tag = AT::avrc_connected;
        ctx.dispatch(act);
    }
    if (s_avrc_just_disconnected.exchange(false, std::memory_order_relaxed)) {
        bt_classic_controller::action act{};
        act.tag = AT::avrc_disconnected;
        ctx.dispatch(act);
    }

    // ── Consume track metadata ────────────────────────────────────────────────
    if (s_track_pending.exchange(false, std::memory_order_relaxed)) {
        bt_classic_controller::action act{};
        act.tag = AT::track_changed;
        taskENTER_CRITICAL(&s_track_mux);
        std::strncpy(act.track.title,  s_pend_title,  127);
        std::strncpy(act.track.artist, s_pend_artist, 127);
        taskEXIT_CRITICAL(&s_track_mux);
        ctx.dispatch(act);
    }

    // ── Activity / deep sleep ─────────────────────────────────────────────────
    if (s_activity_pending.exchange(false, std::memory_order_relaxed)) {
        s_last_activity_ms = now_ms;
    }
    if (kSleepTimeoutMs > 0u && now_ms - s_last_activity_ms >= kSleepTimeoutMs) {
        enter_deep_sleep();
    }

    // ── Periodic metadata poll (every 5 s while AVRC connected) ─────────────────
    // Belt-and-suspenders for platforms that register notifications but never
    // fire them (e.g. Linux/BlueZ + Spotify).  Notifications remain subscribed;
    // this poll catches whatever they miss.
    static std::uint32_t s_last_meta_poll_ms = 0;
    if (state.avrc_connected && now_ms - s_last_meta_poll_ms >= 5000u) {
        s_last_meta_poll_ms = now_ms;
        // Reset accumulator so stale half-received data doesn't block dispatch.
        taskENTER_CRITICAL(&s_track_mux);
        s_pend_title_valid  = false;
        s_pend_artist_valid = false;
        taskEXIT_CRITICAL(&s_track_mux);
        // Use a fixed tl=15 for poll requests so they don't collide with the
        // notification-driven alloc_tl() sequence (which cycles 1–14).
        esp_avrc_ct_send_metadata_cmd(0x0Fu,
            ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST);
    }

    // ── Battery sampling (every 30 s) ─────────────────────────────────────────
    static std::uint32_t s_last_battery_ms = 0;
    if (s_battery.enabled() && now_ms - s_last_battery_ms >= 30'000u) {
        s_last_battery_ms = now_ms;
        (void)s_battery.sample_pct();  // logged inside; no BT Classic battery service
    }

#ifdef CONFIG_BTCC_DISPLAY_ENABLED
    display_update(state);
#else
    (void)state;
#endif
}

#else  // host stub ──────────────────────────────────────────────────────────

void on_init_host(blusys::app_ctx &ctx, blusys::app_fx & /*fx*/,
                  bt_classic_controller::app_state & /*state*/)
{
    s_ctx = &ctx;
    bt_classic_controller::set_hid_send(hid_send_adapter);
    BLUSYS_LOGI(kTag, "bt_classic_controller host stub — no BT available");
}

void on_tick_host(blusys::app_ctx &ctx, blusys::app_fx & /*fx*/,
                  bt_classic_controller::app_state & /*state*/,
                  std::uint32_t /*now_ms*/)
{
    s_ctx = &ctx;
}

#endif  // ESP_PLATFORM

// ── App spec ──────────────────────────────────────────────────────────────────
const blusys::app_spec<bt_classic_controller::app_state,
                       bt_classic_controller::action> spec{
    .initial_state  = {},
    .update         = bt_classic_controller::update,
#ifdef ESP_PLATFORM
    .on_init        = on_init_device,
    .on_tick        = on_tick_device,
#else
    .on_init        = on_init_host,
    .on_tick        = on_tick_host,
#endif
    .tick_period_ms = 50,
    .capabilities   = nullptr,  // no blusys capabilities — all BT managed directly
};

}  // namespace

}  // namespace bt_classic_controller::system

BLUSYS_APP_HEADLESS(bt_classic_controller::system::spec)
