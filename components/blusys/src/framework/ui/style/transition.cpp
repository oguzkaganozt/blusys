#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/style/transition.hpp"
#include "blusys/framework/ui/style/theme.hpp"

namespace blusys {

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

namespace {

void shell_opa_anim_exec(void *var, int32_t v)
{
    lv_obj_set_style_opa(static_cast<lv_obj_t *>(var), static_cast<lv_opa_t>(v), 0);
}

}  // namespace

void shell_content_enter_anim(lv_obj_t *content, const transition_spec &spec)
{
    if (content == nullptr) {
        return;
    }
    if (spec.type == transition_type::none || spec.duration == 0) {
        lv_obj_set_style_opa(content, LV_OPA_COVER, 0);
        return;
    }

    lv_obj_set_style_opa(content, LV_OPA_TRANSP, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, content);
    lv_anim_set_exec_cb(&a, shell_opa_anim_exec);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&a, spec.duration);
    if (spec.path != nullptr) {
        lv_anim_set_path_cb(&a, spec.path);
    }
    lv_anim_start(&a);
}

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
