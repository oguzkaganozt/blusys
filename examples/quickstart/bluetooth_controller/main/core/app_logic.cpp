#include "core/app_logic.hpp"

#include "blusys/hal/log.h"

namespace bluetooth_controller {

namespace {

constexpr const char *kTag = "bt_controller";

constexpr std::uint16_t kUsageVolumeUp   = 0x00E9;
constexpr std::uint16_t kUsageVolumeDown = 0x00EA;
constexpr std::uint16_t kUsageMute       = 0x00E2;

hid_send_fn s_hid_send = nullptr;
led_set_fn  s_led_set  = nullptr;

}  // namespace

void set_hid_send(hid_send_fn fn) { s_hid_send = fn; }
void set_led_setter(led_set_fn fn) { s_led_set = fn; }

const char *op_state_name(op_state s)
{
    switch (s) {
    case op_state::idle:        return "idle";
    case op_state::advertising: return "advertising";
    case op_state::connected:   return "connected";
    }
    return "?";
}

namespace {

void emit(std::uint16_t usage, bool pressed)
{
    if (s_hid_send != nullptr) {
        s_hid_send(usage, pressed);
    }
}

void apply_led(const app_state &state)
{
    if (s_led_set != nullptr) {
        s_led_set(state.ble_connected);
    }
}

}  // namespace

void update(blusys::app_ctx & /*ctx*/, app_state &state, const action &event)
{
    using CET = blusys::capability_event_tag;

    switch (event.tag) {
    case action_tag::capability_event:
        switch (event.cap_event.tag) {
        case CET::ble_hid_advertising_started:
            state.phase = state.ble_connected ? op_state::connected
                                               : op_state::advertising;
            BLUSYS_LOGI(kTag, "advertising — waiting for pairing");
            break;
        case CET::ble_hid_client_connected:
            state.ble_connected = true;
            state.phase         = op_state::connected;
            BLUSYS_LOGI(kTag, "client connected");
            break;
        case CET::ble_hid_client_disconnected:
            state.ble_connected = false;
            state.phase         = op_state::advertising;
            BLUSYS_LOGI(kTag, "client disconnected — re-advertising");
            break;
        case CET::ble_hid_ready:
            if (state.phase == op_state::idle) {
                state.phase = op_state::advertising;
            }
            break;
        default:
            break;
        }
        apply_led(state);
        break;

    case action_tag::button:
        ++state.press_count;
        switch (event.btn) {
        case intent::vol_up_press:
            emit(kUsageVolumeUp, true);
            if (state.volume_est < 100) state.volume_est += 5;
            break;
        case intent::vol_up_release:
            emit(kUsageVolumeUp, false);
            break;
        case intent::vol_down_press:
            emit(kUsageVolumeDown, true);
            if (state.volume_est > 0) state.volume_est -= 5;
            break;
        case intent::vol_down_release:
            emit(kUsageVolumeDown, false);
            break;
        case intent::mute_press:
            emit(kUsageMute, true);
            state.muted = !state.muted;
            break;
        case intent::mute_release:
            emit(kUsageMute, false);
            break;
        }
        BLUSYS_LOGI(kTag,
                    "btn #%lu  phase=%s  vol_est=%d%%  muted=%s",
                    static_cast<unsigned long>(state.press_count),
                    op_state_name(state.phase),
                    state.volume_est,
                    state.muted ? "yes" : "no");
        break;
    }
}

}  // namespace bluetooth_controller
