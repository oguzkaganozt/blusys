#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/transition.hpp"
#include "blusys/framework/ui/theme.hpp"

namespace blusys::ui {

namespace {

// Map transition_type to the corresponding LVGL screen load animation.
lv_scr_load_anim_t to_lv_anim(transition_type type)
{
    switch (type) {
    case transition_type::slide_left:  return LV_SCR_LOAD_ANIM_MOVE_LEFT;
    case transition_type::slide_right: return LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    case transition_type::fade:        return LV_SCR_LOAD_ANIM_FADE_IN;
    case transition_type::slide_up:    return LV_SCR_LOAD_ANIM_MOVE_TOP;
    case transition_type::slide_down:  return LV_SCR_LOAD_ANIM_MOVE_BOTTOM;
    default:                           return LV_SCR_LOAD_ANIM_NONE;
    }
}

// Pick the easing function from theme tokens based on transition type.
lv_anim_path_cb_t pick_path(transition_type type, const theme_tokens &t)
{
    switch (type) {
    case transition_type::slide_left:
    case transition_type::slide_up:
        return t.anim_path_enter;
    case transition_type::slide_right:
    case transition_type::slide_down:
        return t.anim_path_exit;
    case transition_type::fade:
        return t.anim_path_standard;
    default:
        return nullptr;
    }
}

}  // namespace

transition_spec resolve_transition(std::uint32_t transition_field)
{
    return resolve_transition(transition_field, transition_type::none);
}

transition_spec resolve_transition(std::uint32_t transition_field,
                                   transition_type default_type)
{
    const auto &t = theme();

    // If animations are globally disabled, always return none.
    if (!t.anim_enabled) {
        return {};
    }

    // Determine the effective type: explicit field takes precedence,
    // then the caller-supplied default for this command type.
    auto type = (transition_field != 0)
                    ? static_cast<transition_type>(transition_field)
                    : default_type;

    if (type == transition_type::none) {
        return {};
    }

    return {
        .type     = type,
        .duration = t.anim_duration_normal,
        .path     = pick_path(type, t),
        .lv_anim  = to_lv_anim(type),
    };
}

}  // namespace blusys::ui

#endif  // BLUSYS_FRAMEWORK_HAS_UI
