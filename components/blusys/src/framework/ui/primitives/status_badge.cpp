#include "blusys/framework/ui/primitives/status_badge.hpp"

#include "blusys/framework/ui/style/theme.hpp"

// Display-only status badge pill. No callbacks, no slot pool.
//
// Internal layout: a small rounded container with a single lv_label child.
// Background color and text color are derived from the badge_level and the
// theme's semantic status colors.

namespace blusys {
namespace {

struct level_colors {
    lv_color_t bg;
    lv_color_t fg;
};

level_colors colors_for(badge_level level)
{
    const auto &t = theme();
    switch (level) {
    case badge_level::success: return {t.color_success, t.color_on_success};
    case badge_level::warning: return {t.color_warning, t.color_on_warning};
    case badge_level::error:   return {t.color_error, t.color_on_error};
    case badge_level::info:
    default:                   return {t.color_info, t.color_on_surface};
    }
}

void apply_level(lv_obj_t *badge, badge_level level)
{
    auto c = colors_for(level);
    lv_obj_set_style_bg_color(badge, c.bg, 0);
    // Update child label text color.
    lv_obj_t *label = lv_obj_get_child(badge, 0);
    if (label != nullptr) {
        lv_obj_set_style_text_color(label, c.fg, 0);
    }
}

}  // namespace

lv_obj_t *status_badge_create(lv_obj_t *parent, const status_badge_config &config)
{
    const auto &t = theme();

    // Pill container.
    lv_obj_t *badge = lv_obj_create(parent);
    lv_obj_set_width(badge, LV_SIZE_CONTENT);
    lv_obj_set_height(badge, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(badge, 999, 0);  // pill shape
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_pad_left(badge, t.spacing_sm, 0);
    lv_obj_set_style_pad_right(badge, t.spacing_sm, 0);
    lv_obj_set_style_pad_top(badge, t.spacing_xs, 0);
    lv_obj_set_style_pad_bottom(badge, t.spacing_xs, 0);
    lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    // Text label inside.
    lv_obj_t *label = lv_label_create(badge);
    lv_obj_set_style_text_font(label, t.font_body_sm, 0);
    lv_label_set_text(label, config.text != nullptr ? config.text : "");

    apply_level(badge, config.level);

    return badge;
}

void status_badge_set_text(lv_obj_t *badge, const char *text)
{
    if (badge == nullptr) {
        return;
    }
    lv_obj_t *label = lv_obj_get_child(badge, 0);
    if (label != nullptr) {
        lv_label_set_text(label, text != nullptr ? text : "");
    }
}

void status_badge_set_level(lv_obj_t *badge, badge_level level)
{
    if (badge == nullptr) {
        return;
    }
    apply_level(badge, level);
}

}  // namespace blusys
