#pragma once

#include <cstdint>

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/event.hpp"

namespace bluetooth_controller {

enum class op_state : std::uint8_t {
    idle,
    advertising,
    connected,
};

/// Active control mode — determines what CW/CCW/short-press do.
enum class ctrl_mode : std::uint8_t {
    volume,     ///< CW=Vol+   CCW=Vol-  short-press=Mute
    media,      ///< CW=Next   CCW=Prev  short-press=Play/Pause
    brightness, ///< CW=Bright+ CCW=Bright- short-press=Mute
};

enum class intent : std::uint8_t {
    turn_cw,           ///< Encoder clockwise detent
    turn_ccw,          ///< Encoder counter-clockwise detent
    button_press,      ///< Button pressed (action fires on release if not long)
    button_release,    ///< Button released
    button_long_press, ///< Button held past threshold → cycle mode
};

enum class action_tag : std::uint8_t {
    capability_event,
    button,
};

struct action {
    action_tag               tag = action_tag::capability_event;
    intent                   btn = intent::turn_cw;
    blusys::capability_event cap_event{};
};

struct app_state {
    op_state      phase            = op_state::idle;
    bool          ble_connected    = false;
    ctrl_mode     mode             = ctrl_mode::volume;
    bool          button_held      = false; ///< True while button is physically held
    bool          long_press_fired = false; ///< True if long-press already triggered
    int           volume_est       = 50;
    bool          muted            = false;
    std::uint32_t press_count      = 0;
};

void update(blusys::app_ctx &ctx, app_state &state, const action &event);

/// Installed by platform/app_main.cpp; reducer talks to the radio only here.
using hid_send_fn = void (*)(std::uint16_t usage, bool pressed);
void set_hid_send(hid_send_fn fn);

/// Platform-provided LED control. True = client connected, false = advertising.
using led_set_fn = void (*)(bool on);
void set_led_setter(led_set_fn fn);

const char *op_state_name(op_state s);
const char *ctrl_mode_name(ctrl_mode m);

}  // namespace bluetooth_controller
