#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include <cstdint>

#include "lvgl.h"

namespace blusys::ui {

// Screen transition types. These map to LVGL's built-in screen load
// animations. The value is stored in route_command::transition as a
// uint32_t — cast to this enum to interpret it.
enum class transition_type : std::uint32_t {
    none        = 0,  // instant swap (current behavior; also used when anim_enabled == false)
    slide_left  = 1,  // new screen slides in from right, old slides out left (push)
    slide_right = 2,  // reverse of slide_left (pop / back)
    fade        = 3,  // cross-fade
    slide_up    = 4,  // new screen slides up (modal-style push)
    slide_down  = 5,  // reverse of slide_up (dismiss)
};

// Resolved transition specification, ready for LVGL consumption.
struct transition_spec {
    transition_type    type     = transition_type::none;
    std::uint16_t      duration = 0;      // milliseconds; 0 = theme default
    lv_anim_path_cb_t  path    = nullptr; // nullptr = theme default
    lv_scr_load_anim_t lv_anim = LV_SCR_LOAD_ANIM_NONE;
};

// Resolve a uint32_t transition field (from route_command) into a
// concrete transition_spec. Reads theme().anim_enabled and theme()
// duration/path tokens. If animations are disabled, always returns none.
transition_spec resolve_transition(std::uint32_t transition_field);

// Resolve a transition with a default type to use when the transition
// field is 0 and animations are enabled (e.g., slide_left for push,
// slide_right for pop).
transition_spec resolve_transition(std::uint32_t transition_field,
                                   transition_type default_type);

}  // namespace blusys::ui

#endif  // BLUSYS_FRAMEWORK_HAS_UI
