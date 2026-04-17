#include "core/app_logic.hpp"

#include "blusys/hal/log.h"

#include <optional>

namespace bluetooth_controller {

namespace {

constexpr const char *kTag = "bt_controller";

// Consumer Control HID usages (Consumer page 0x0C)
constexpr std::uint16_t kUsageVolumeUp    = 0x00E9;
constexpr std::uint16_t kUsageVolumeDown  = 0x00EA;
constexpr std::uint16_t kUsageMute        = 0x00E2;
constexpr std::uint16_t kUsagePlayPause   = 0x00CD;
constexpr std::uint16_t kUsageNextTrack   = 0x00B5;
constexpr std::uint16_t kUsagePrevTrack   = 0x00B6;
constexpr std::uint16_t kUsageBrightUp    = 0x006F;
constexpr std::uint16_t kUsageBrightDown  = 0x0070;

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

const char *ctrl_mode_name(ctrl_mode m)
{
    switch (m) {
    case ctrl_mode::volume:     return "volume";
    case ctrl_mode::media:      return "media";
    case ctrl_mode::brightness: return "brightness";
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

// Emit a press + immediate release (one-shot consumer usage).
void click(std::uint16_t usage)
{
    emit(usage, true);
    emit(usage, false);
}

void apply_led(const app_state &state)
{
    if (s_led_set != nullptr) {
        s_led_set(state.ble_connected);
    }
}

ctrl_mode next_mode(ctrl_mode m)
{
    switch (m) {
    case ctrl_mode::volume:     return ctrl_mode::media;
    case ctrl_mode::media:      return ctrl_mode::brightness;
    case ctrl_mode::brightness: return ctrl_mode::volume;
    }
    return ctrl_mode::volume;
}

}  // namespace

std::optional<action> on_event(blusys::event event, app_state &state)
{
    (void)state;
    if (event.kind != blusys::app_event_kind::integration) {
        return std::nullopt;
    }

    blusys::capability_event ce{};
    if (!blusys::map_integration_event(event.id, event.code, &ce)) {
        return std::nullopt;
    }
    ce.payload = event.payload;
    return action{.tag = action_tag::capability_event, .cap_event = ce};
}

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
        switch (event.btn) {

        // ── Rotation ─────────────────────────────────────────────────────────
        case intent::turn_cw:
            ++state.press_count;
            switch (state.mode) {
            case ctrl_mode::volume:
                click(kUsageVolumeUp);
                if (state.volume_est < 100) state.volume_est += 5;
                break;
            case ctrl_mode::media:      click(kUsageNextTrack);  break;
            case ctrl_mode::brightness: click(kUsageBrightUp);   break;
            }
            break;

        case intent::turn_ccw:
            ++state.press_count;
            switch (state.mode) {
            case ctrl_mode::volume:
                click(kUsageVolumeDown);
                if (state.volume_est > 0) state.volume_est -= 5;
                break;
            case ctrl_mode::media:      click(kUsagePrevTrack);  break;
            case ctrl_mode::brightness: click(kUsageBrightDown); break;
            }
            break;

        // ── Button: deferred short-press to avoid collision with long-press ──
        case intent::button_press:
            state.button_held      = true;
            state.long_press_fired = false;
            break;

        case intent::button_long_press:
            // Cycle mode; short-press action suppressed on release.
            state.long_press_fired = true;
            state.mode = next_mode(state.mode);
            BLUSYS_LOGI(kTag, "mode → %s", ctrl_mode_name(state.mode));
            break;

        case intent::button_release:
            if (state.button_held && !state.long_press_fired) {
                // Short press confirmed — perform context action.
                ++state.press_count;
                switch (state.mode) {
                case ctrl_mode::volume:
                    click(kUsageMute);
                    state.muted = !state.muted;
                    break;
                case ctrl_mode::media:      click(kUsagePlayPause); break;
                case ctrl_mode::brightness: click(kUsageMute);      break;
                }
            }
            state.button_held      = false;
            state.long_press_fired = false;
            break;
        }

        BLUSYS_LOGI(kTag,
                    "btn #%lu  mode=%-10s  phase=%-11s  vol_est=%3d%%  muted=%s",
                    static_cast<unsigned long>(state.press_count),
                    ctrl_mode_name(state.mode),
                    op_state_name(state.phase),
                    state.volume_est,
                    state.muted ? "yes" : "no");
        break;
    }
}

}  // namespace bluetooth_controller
