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

enum class intent : std::uint8_t {
    vol_up_press,
    vol_up_release,
    vol_down_press,
    vol_down_release,
    mute_press,
    mute_release,
};

enum class action_tag : std::uint8_t {
    capability_event,
    button,
};

struct action {
    action_tag              tag = action_tag::capability_event;
    intent                  btn = intent::vol_up_press;
    blusys::capability_event cap_event{};
};

struct app_state {
    op_state      phase         = op_state::idle;
    bool          ble_connected = false;
    // Best-effort mirror — the device never reads host volume; this just
    // tracks the direction of our own button presses for the log line.
    int           volume_est    = 50;
    bool          muted         = false;
    std::uint32_t press_count   = 0;
};

void update(blusys::app_ctx &ctx, app_state &state, const action &event);

// Callback installed by platform/app_main.cpp so the pure reducer stays free
// of capability references — reducer talks to the radio only through this.
using hid_send_fn = void (*)(std::uint16_t usage, bool pressed);
void set_hid_send(hid_send_fn fn);

// Platform-provided LED controller. status is true whenever a client is
// connected; advertising-without-client is handled via on_tick blinking.
using led_set_fn = void (*)(bool on);
void set_led_setter(led_set_fn fn);

const char *op_state_name(op_state s);

}  // namespace bluetooth_controller
