// Bluetooth volume-controller platform layer.
//
// Wires three GPIO buttons + a status LED to the shared app_logic reducer,
// and exposes the ble_hid_device_capability as the single outbound channel.
// The reducer stays capability-free and talks to the radio via the hid_send_fn
// pointer installed here.

#include "core/app_logic.hpp"

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/ble_hid_device.hpp"
#include "blusys/hal/log.h"

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#include "blusys/drivers/button.h"
#include "blusys/hal/gpio.h"
#endif

#include <cstdint>

namespace bluetooth_controller::system {

namespace {

constexpr const char *kTag = "bt_controller";

#ifdef ESP_PLATFORM
constexpr int kVolUpPin    = CONFIG_BTCTRL_VOL_UP_GPIO;
constexpr int kVolDownPin  = CONFIG_BTCTRL_VOL_DOWN_GPIO;
constexpr int kMutePin     = CONFIG_BTCTRL_MUTE_GPIO;
constexpr int kStatusLedPin = CONFIG_BTCTRL_STATUS_LED_GPIO;
#ifdef CONFIG_BTCTRL_STATUS_LED_ACTIVE_LOW
constexpr bool kStatusLedActiveLow = true;
#else
constexpr bool kStatusLedActiveLow = false;
#endif
constexpr const char *kDeviceName = CONFIG_BTCTRL_DEVICE_NAME;
#else
constexpr const char *kDeviceName = "Blusys Controller";
#endif

blusys::ble_hid_device_capability hid_device{{
    .device_name         = kDeviceName,
    .vendor_id           = 0x303A,  // Espressif — reasonable default for dev boards.
    .product_id          = 0x8001,
    .version             = 0x0100,
    .initial_battery_pct = 100,
}};

blusys::capability_list capabilities{&hid_device};

// ── Plumbing between buttons and the reducer ────────────────────────────────
blusys::app_ctx *s_ctx = nullptr;

void dispatch_button(bluetooth_controller::intent btn)
{
    if (s_ctx == nullptr) {
        return;
    }
    bluetooth_controller::action act{};
    act.tag = bluetooth_controller::action_tag::button;
    act.btn = btn;
    s_ctx->dispatch(act);
}

// ── Tick-driven "fake button" simulator ─────────────────────────────────────
// Emits 3× Vol+ clicks, 400 ms between clicks, then a 600 ms gap, then
// 3× Vol-. Each click is a press / release pair 80 ms apart. The sequence
// restarts every time a client (re)connects so the whole cycle is observable
// on pairing and on every reconnect.
struct click_simulator {
    static constexpr int           kClicksPerPhase = 3;
    static constexpr std::uint32_t kPressDurationMs = 80;
    static constexpr std::uint32_t kInterClickGapMs = 400;
    static constexpr std::uint32_t kPhaseGapMs      = 600;

    enum class phase : std::uint8_t { idle, volume_up, phase_gap, volume_down, done };

    phase         phase_          = phase::idle;
    int           clicks_emitted_ = 0;
    bool          pressed_        = false;
    std::uint32_t next_edge_ms_   = 0;
    bool          was_connected_  = false;

    void arm(std::uint32_t now_ms)
    {
        phase_          = phase::volume_up;
        clicks_emitted_ = 0;
        pressed_        = false;
        next_edge_ms_   = now_ms + kInterClickGapMs;  // small lead-in
        BLUSYS_LOGI(kTag, "simulator ARMED — 3x Vol+ then 3x Vol- starting in %u ms",
                    static_cast<unsigned>(kInterClickGapMs));
    }

    void reset()
    {
        phase_          = phase::idle;
        clicks_emitted_ = 0;
        pressed_        = false;
        next_edge_ms_   = 0;
    }

    void tick(std::uint32_t now_ms, bool connected)
    {
        if (connected && !was_connected_) {
            arm(now_ms);
        } else if (!connected && was_connected_) {
            reset();
        }
        was_connected_ = connected;

        if (!connected || phase_ == phase::idle || phase_ == phase::done) {
            return;
        }
        if (static_cast<std::int32_t>(now_ms - next_edge_ms_) < 0) {
            return;
        }

        auto click = [&](bluetooth_controller::intent press,
                         bluetooth_controller::intent release) {
            if (!pressed_) {
                dispatch_button(press);
                pressed_ = true;
                next_edge_ms_ = now_ms + kPressDurationMs;
            } else {
                dispatch_button(release);
                pressed_ = false;
                ++clicks_emitted_;
                next_edge_ms_ = now_ms + kInterClickGapMs;
            }
        };

        switch (phase_) {
        case phase::volume_up:
            click(bluetooth_controller::intent::vol_up_press,
                  bluetooth_controller::intent::vol_up_release);
            if (clicks_emitted_ >= kClicksPerPhase && !pressed_) {
                phase_          = phase::phase_gap;
                clicks_emitted_ = 0;
                next_edge_ms_   = now_ms + kPhaseGapMs;
                BLUSYS_LOGI(kTag, "simulator — Vol+ phase done, pausing %u ms",
                            static_cast<unsigned>(kPhaseGapMs));
            }
            break;
        case phase::phase_gap:
            phase_        = phase::volume_down;
            next_edge_ms_ = now_ms;  // start immediately
            break;
        case phase::volume_down:
            click(bluetooth_controller::intent::vol_down_press,
                  bluetooth_controller::intent::vol_down_release);
            if (clicks_emitted_ >= kClicksPerPhase && !pressed_) {
                phase_ = phase::done;
                BLUSYS_LOGI(kTag, "simulator — sequence complete (3x Vol+ + 3x Vol-)");
            }
            break;
        case phase::idle:
        case phase::done:
            break;
        }
    }
};

click_simulator s_simulator{};

const char *usage_name(std::uint16_t usage)
{
    switch (usage) {
    case 0x00E9: return "VolumeUp";
    case 0x00EA: return "VolumeDown";
    case 0x00E2: return "Mute";
    case 0x00CD: return "PlayPause";
    default:     return "?";
    }
}

void hid_send_adapter(std::uint16_t usage, bool pressed)
{
    auto err = hid_device.send_consumer(usage, pressed);
    bool connected = hid_device.is_connected();
    BLUSYS_LOGI(kTag, "HID → %s (0x%04x) %s  connected=%d  rc=%d",
                usage_name(usage), usage, pressed ? "press  " : "release",
                connected ? 1 : 0, static_cast<int>(err));
}

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

struct button_slot {
    blusys_button_t               *handle      = nullptr;
    bluetooth_controller::intent   press_intent  = bluetooth_controller::intent::vol_up_press;
    bluetooth_controller::intent   release_intent = bluetooth_controller::intent::vol_up_release;
};

button_slot s_btn_vol_up{
    .press_intent   = bluetooth_controller::intent::vol_up_press,
    .release_intent = bluetooth_controller::intent::vol_up_release,
};
button_slot s_btn_vol_down{
    .press_intent   = bluetooth_controller::intent::vol_down_press,
    .release_intent = bluetooth_controller::intent::vol_down_release,
};
button_slot s_btn_mute{
    .press_intent   = bluetooth_controller::intent::mute_press,
    .release_intent = bluetooth_controller::intent::mute_release,
};

void on_button_event(blusys_button_t * /*button*/,
                     blusys_button_event_t event,
                     void *user_ctx)
{
    auto *slot = static_cast<button_slot *>(user_ctx);
    if (slot == nullptr) {
        return;
    }
    switch (event) {
    case BLUSYS_BUTTON_EVENT_PRESS:
        dispatch_button(slot->press_intent);
        break;
    case BLUSYS_BUTTON_EVENT_RELEASE:
        dispatch_button(slot->release_intent);
        break;
    default:
        break;
    }
}

blusys_err_t open_button(int pin, button_slot &slot)
{
    blusys_button_config_t cfg{};
    cfg.pin        = pin;
    cfg.pull_mode  = BLUSYS_GPIO_PULL_UP;
    cfg.active_low = true;
    cfg.debounce_ms = 0;  // driver default (50 ms)
    blusys_err_t err = blusys_button_open(&cfg, &slot.handle);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "button_open(pin=%d) failed: %d", pin, static_cast<int>(err));
        return err;
    }
    return blusys_button_set_callback(slot.handle, on_button_event, &slot);
}

void on_init_device(blusys::app_ctx &ctx, blusys::app_services & /*svc*/,
                    app_state & /*state*/)
{
    s_ctx = &ctx;
    bluetooth_controller::set_hid_send(hid_send_adapter);
    bluetooth_controller::set_led_setter(led_set);
    status_led_init();
    open_button(kVolUpPin,   s_btn_vol_up);
    open_button(kVolDownPin, s_btn_vol_down);
    open_button(kMutePin,    s_btn_mute);
    BLUSYS_LOGI(kTag,
                "bluetooth controller up — pins: up=%d down=%d mute=%d led=%d",
                kVolUpPin, kVolDownPin, kMutePin, kStatusLedPin);
#ifdef CONFIG_BTCTRL_SIMULATE_ON_CONNECT
    BLUSYS_LOGI(kTag, "auto-simulation enabled — 3x Vol+ then 3x Vol- on each connect");
#endif
}

#ifdef CONFIG_BTCTRL_SIMULATE_ON_CONNECT
void on_tick_device(blusys::app_ctx & /*ctx*/, blusys::app_services & /*svc*/,
                    app_state & /*state*/, std::uint32_t now_ms)
{
    // Gate on is_ready() (connected + Input Report CCCD=1), NOT on bare GAP
    // connect. Linux's BlueZ subscribes a good 2-3 s after GAP connect on
    // first pair; any click sent before that goes nowhere.
    s_simulator.tick(now_ms, hid_device.is_ready());
}
#endif

#else  // host build

void led_set(bool on)
{
    BLUSYS_LOGI(kTag, "[led] %s", on ? "solid (connected)" : "blink (advertising)");
}

// Host tick: reuse the same simulator the device uses so the pipeline is
// exercised identically under both builds.
void on_tick_host(blusys::app_ctx &ctx, blusys::app_services & /*svc*/,
                  app_state &state, std::uint32_t now_ms)
{
    s_ctx = &ctx;
    s_simulator.tick(now_ms, state.ble_connected);
}

void on_init_host(blusys::app_ctx &ctx, blusys::app_services & /*svc*/,
                  app_state & /*state*/)
{
    s_ctx = &ctx;
    bluetooth_controller::set_hid_send(hid_send_adapter);
    bluetooth_controller::set_led_setter(led_set);
    BLUSYS_LOGI(kTag, "bluetooth controller host stub — simulating button presses");
}

#endif  // ESP_PLATFORM

const blusys::app_spec<app_state, action> spec{
    .initial_state = {},
    .update        = update,
#ifdef ESP_PLATFORM
    .on_init       = on_init_device,
  #ifdef CONFIG_BTCTRL_SIMULATE_ON_CONNECT
    .on_tick       = on_tick_device,
  #endif
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
