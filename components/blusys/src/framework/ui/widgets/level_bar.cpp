#include "blusys/framework/ui/widgets/level_bar.hpp"

#include <cstdio>

#include "blusys/framework/ui/style/theme.hpp"

// Display-only level bar wrapping `lv_bar`. Same child layout contract as
// `bu_progress`: child 0 is the bar; optional child 1 is a caption label.

namespace blusys {
namespace {

constexpr int kChildBar   = 0;
constexpr int kChildLabel = 1;

lv_obj_t *bar_from(lv_obj_t *level_bar)
{
    return lv_obj_get_child(level_bar, kChildBar);
}

lv_obj_t *label_from(lv_obj_t *level_bar)
{
    uint32_t count = lv_obj_get_child_count(level_bar);
    return count > 1 ? lv_obj_get_child(level_bar, kChildLabel) : nullptr;
}

void apply_bar_theme(lv_obj_t *bar)
{
    const auto &t = theme();

    lv_obj_set_style_bg_color(bar, t.color_outline_variant, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, t.radius_button, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);

    lv_obj_set_style_bg_color(bar, t.color_accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, t.radius_button, LV_PART_INDICATOR);
}

}  // namespace

lv_obj_t *level_bar_create(lv_obj_t *parent, const level_bar_config &config)
{
    const auto &t = theme();

    lv_coord_t bar_h = t.spacing_xs * 2;
    if (bar_h < 4) {
        bar_h = 4;
    }

    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_pad_row(container, t.spacing_xs, 0);
    lv_obj_set_width(container, LV_PCT(100));
    lv_obj_set_height(container, LV_SIZE_CONTENT);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bar = lv_bar_create(container);
    apply_bar_theme(bar);
    lv_bar_set_range(bar, config.min, config.max);
    lv_bar_set_value(bar, config.initial, LV_ANIM_OFF);
    lv_obj_set_width(bar, LV_PCT(100));
    lv_obj_set_height(bar, bar_h);

    if (config.label != nullptr) {
        lv_obj_t *label = lv_label_create(container);
        lv_label_set_text(label, config.label);
        lv_obj_set_style_text_color(label, t.color_on_surface, 0);
        lv_obj_set_style_text_font(label, t.font_body_sm, 0);
    }

    return container;
}

void level_bar_set_value(lv_obj_t *level_bar, int32_t value)
{
    if (level_bar == nullptr) {
        return;
    }
    lv_obj_t *bar = bar_from(level_bar);
    if (bar == nullptr) {
        return;
    }
    lv_bar_set_value(bar, value, LV_ANIM_OFF);
}

int32_t level_bar_get_value(lv_obj_t *level_bar)
{
    if (level_bar == nullptr) {
        return 0;
    }
    lv_obj_t *bar = bar_from(level_bar);
    return bar != nullptr ? lv_bar_get_value(bar) : 0;
}

void level_bar_set_range(lv_obj_t *level_bar, int32_t min, int32_t max)
{
    if (level_bar == nullptr) {
        return;
    }
    lv_obj_t *bar = bar_from(level_bar);
    if (bar == nullptr) {
        return;
    }
    lv_bar_set_range(bar, min, max);
}

void level_bar_set_label(lv_obj_t *level_bar, const char *text)
{
    if (level_bar == nullptr) {
        return;
    }
    lv_obj_t *label = label_from(level_bar);
    if (label != nullptr && text != nullptr) {
        lv_label_set_text(label, text);
    }
}

}  // namespace blusys
