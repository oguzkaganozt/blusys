#include "core/app_logic.hpp"
#include "blusys/blusys.hpp"


namespace bt_classic_controller {

namespace {

constexpr const char *kTag = "btcc";

// Consumer Control HID usages (Consumer page 0x0C) — same as bluetooth_controller.
constexpr std::uint16_t kUsageVolumeUp   = 0x00E9;
constexpr std::uint16_t kUsageVolumeDown = 0x00EA;
constexpr std::uint16_t kUsageMute       = 0x00E2;
constexpr std::uint16_t kUsagePlayPause  = 0x00CD;
constexpr std::uint16_t kUsageNextTrack  = 0x00B5;
constexpr std::uint16_t kUsagePrevTrack  = 0x00B6;
constexpr std::uint16_t kUsageBrightUp   = 0x006F;
constexpr std::uint16_t kUsageBrightDown = 0x0070;

hid_send_fn s_hid_send = nullptr;

}  // namespace

void set_hid_send(hid_send_fn fn) { s_hid_send = fn; }

const char *op_state_name(op_state s)
{
    switch (s) {
    case op_state::idle:         return "idle";
    case op_state::discoverable: return "discoverable";
    case op_state::connected:    return "connected";
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

void click(std::uint16_t usage)
{
    emit(usage, true);
    emit(usage, false);
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

void update(blusys::app_ctx & /*ctx*/, app_state &state, const action &event)
{
    switch (event.tag) {

    // ── Bluetooth connection lifecycle ────────────────────────────────────────
    case action_tag::hid_connected:
        state.hid_connected = true;
        state.phase         = op_state::connected;
        BLUSYS_LOGI(kTag, "HID connected");
        break;

    case action_tag::hid_disconnected:
        state.hid_connected  = false;
        state.avrc_connected = false;
        state.phase          = op_state::discoverable;
        state.track          = {};  // clear stale track on disconnect
        BLUSYS_LOGI(kTag, "HID disconnected — back to discoverable");
        break;

    case action_tag::avrc_connected:
        state.avrc_connected = true;
        BLUSYS_LOGI(kTag, "AVRC connected — track metadata available");
        break;

    case action_tag::avrc_disconnected:
        state.avrc_connected = false;
        state.track          = {};
        BLUSYS_LOGI(kTag, "AVRC disconnected");
        break;

    case action_tag::track_changed:
        state.track = event.track;
        BLUSYS_LOGI(kTag, "track: \"%s\" — \"%s\"",
                    state.track.title, state.track.artist);
        break;

    // ── Encoder input ─────────────────────────────────────────────────────────
    case action_tag::button:
        switch (event.btn) {

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

        case intent::button_press:
            state.button_held      = true;
            state.long_press_fired = false;
            break;

        case intent::button_long_press:
            state.long_press_fired = true;
            state.mode = next_mode(state.mode);
            BLUSYS_LOGI(kTag, "mode → %s", ctrl_mode_name(state.mode));
            break;

        case intent::button_release:
            if (state.button_held && !state.long_press_fired) {
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
                    "btn #%lu  mode=%-10s  phase=%-12s  vol=%3d%%  muted=%s",
                    static_cast<unsigned long>(state.press_count),
                    ctrl_mode_name(state.mode),
                    op_state_name(state.phase),
                    state.volume_est,
                    state.muted ? "yes" : "no");
        break;
    }
}

}  // namespace bt_classic_controller
