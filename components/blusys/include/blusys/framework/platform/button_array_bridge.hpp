#pragma once

// Button array input bridge — maps hardware buttons to framework intents.
//
// Products define up to kMaxButtons buttons, each with a GPIO config
// and a mapping to a framework intent for press and long-press events.
// The bridge opens each button via the HAL driver and posts the mapped
// intent to the framework runtime.
//
// Product code never touches this directly — the device entry path
// creates the bridge when the device_profile has button_count > 0.

#if defined(BLUSYS_FRAMEWORK_HAS_UI)

#include "blusys/drivers/button.h"
#include "blusys/hal/error.h"
#include "blusys/framework/events/event.hpp"
#include "blusys/framework/events/event_queue.hpp"

#include <cstddef>

namespace blusys {

static constexpr std::size_t kMaxButtons = 8;

// Mapping from a hardware button to framework intents.
// Typical mapping: confirm / cancel — map `cancel` to `navigate_back` in your `on_event`.
struct button_mapping {
    blusys_button_config_t      button_config;
    blusys::intent   on_press      = blusys::intent::confirm;
    blusys::intent   on_long_press = blusys::intent::cancel;
};

struct button_array_config {
    button_mapping                  buttons[kMaxButtons];
    std::size_t                     count             = 0;
    blusys::runtime     *framework_runtime = nullptr;
};

struct button_array_bridge {
    blusys_button_t *buttons[kMaxButtons] = {};
    std::size_t      count                = 0;
};

blusys_err_t button_array_open(const button_array_config &config,
                                button_array_bridge *out);

void button_array_close(button_array_bridge *bridge);

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
