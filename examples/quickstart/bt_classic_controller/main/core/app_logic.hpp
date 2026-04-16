#pragma once

#include <cstdint>

#include "blusys/framework/app/app.hpp"

namespace bt_classic_controller {

// ── Track metadata (filled from AVRCP) ───────────────────────────────────────
struct track_info {
    char title[128]  = {};
    char artist[128] = {};
};

/// Device connection phase.
enum class op_state : std::uint8_t {
    idle,          ///< BT stack starting up
    discoverable,  ///< Connectable + general discoverable, waiting for host
    connected,     ///< HID host connected
};

/// Active control mode — determines what CW/CCW/short-press do.
enum class ctrl_mode : std::uint8_t {
    volume,     ///< CW=Vol+    CCW=Vol-   short-press=Mute
    media,      ///< CW=Next    CCW=Prev   short-press=Play/Pause
    brightness, ///< CW=Bright+ CCW=Bright- short-press=Mute
};

enum class intent : std::uint8_t {
    turn_cw,
    turn_ccw,
    button_press,
    button_release,
    button_long_press,
};

enum class action_tag : std::uint8_t {
    button,
    hid_connected,
    hid_disconnected,
    avrc_connected,
    avrc_disconnected,
    track_changed,
};

struct action {
    action_tag  tag   = action_tag::button;
    intent      btn   = intent::turn_cw;
    track_info  track = {};  ///< Populated only when tag == track_changed.
};

struct app_state {
    op_state      phase            = op_state::idle;
    bool          hid_connected    = false;
    bool          avrc_connected   = false;
    ctrl_mode     mode             = ctrl_mode::volume;
    bool          button_held      = false;
    bool          long_press_fired = false;
    int           volume_est       = 50;
    bool          muted            = false;
    std::uint32_t press_count      = 0;
    track_info    track            = {};
};

void update(blusys::app_ctx &ctx, app_state &state, const action &event);

/// Installed by platform/app_main.cpp — reducer talks to the radio only here.
using hid_send_fn = void (*)(std::uint16_t usage, bool pressed);
void set_hid_send(hid_send_fn fn);

const char *op_state_name(op_state s);
const char *ctrl_mode_name(ctrl_mode m);

}  // namespace bt_classic_controller
