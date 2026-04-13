#pragma once

#include "blusys/app/app.hpp"
#include "blusys/app/capability_event.hpp"

#include <cstdint>

namespace usb_basic {

enum class action_tag : std::uint8_t {
    capability_event,
};

struct action {
    action_tag tag = action_tag::capability_event;
    blusys::app::capability_event cap_event{};
};

struct app_state {
    bool usb_ready = false;
    bool device_ready = false;
    bool device_connected = false;
};

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);

}  // namespace usb_basic
