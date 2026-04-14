#include "blusys/framework/ui/widgets/progress.hpp"

#include <cstdio>

#include "blusys/framework/ui/style/theme.hpp"

// Display-only progress bar wrapping `lv_bar`. No callbacks, no slot pool.
//
// Internal layout: a vertical container holding the bar and an optional
// text label below it. The bar itself optionally shows a percentage label
// rendered as a child `lv_label` centered on the indicator.

namespace blusys {
namespace {

// Keys into lv_obj user_data are not needed here — we use the parent
// container's first and second children by index instead.

constexpr int kChildBar   = 0;
constexpr int kChildLabel = 1;

lv_obj_t *bar_from(lv_obj_t *progress)
{
    return lv_obj_get_child(progress, kChildBar);
}

lv_obj_t *label_from(lv_obj_t *progress)
{
    uint32_t count = lv_obj_get_child_count(progress);
    return count > 1 ? lv_obj_get_child(progress, kChildLabel) : nullptr;
}

void apply_bar_theme(lv_obj_t *bar)
{
    const auto &t = theme();

    // Track (background).
    lv_obj_set_style_bg_color(bar, t.color_disabled, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, t.radius_button, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);

    // Indicator (filled portion).
    lv_obj_set_style_bg_color(bar, t.color_primary, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, t.radius_button, LV_PART_INDICATOR);
}

void update_pct_label(lv_obj_t *bar, int32_t value, int32_t min, int32_t max)
{
    // The percentage label is stored as user_data on the bar itself.
    auto *pct_label = static_cast<lv_obj_t *>(lv_obj_get_user_data(bar));
    if (pct_label == nullptr) {
        return;
    }

    int pct = 0;
    if (max > min) {
        pct = static_cast<int>((static_cast<int64_t>(value - min) * 100) / (max - min));
    }
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%d%%", pct);
    lv_label_set_text(pct_label, buf);
    lv_obj_center(pct_label);
}

}  // namespace

lv_obj_t *progress_create(lv_obj_t *parent, const progress_config &config)
{
    const auto &t = theme();

    // Container: vertical column, no background.
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

    // Bar.
    lv_obj_t *bar = lv_bar_create(container);
    apply_bar_theme(bar);
    lv_bar_set_range(bar, config.min, config.max);
    lv_bar_set_value(bar, config.initial, LV_ANIM_OFF);
    lv_obj_set_width(bar, LV_PCT(100));
    lv_obj_set_height(bar, t.spacing_md);

    // Optional percentage label centered on the bar.
    if (config.show_pct) {
        lv_obj_t *pct_label = lv_label_create(bar);
        lv_obj_set_style_text_color(pct_label, t.color_on_primary, 0);
        lv_obj_set_style_text_font(pct_label, t.font_body_sm, 0);
        lv_obj_set_user_data(bar, pct_label);
        update_pct_label(bar, config.initial, config.min, config.max);
    }

    // Optional descriptive text label.
    if (config.label != nullptr) {
        lv_obj_t *label = lv_label_create(container);
        lv_label_set_text(label, config.label);
        lv_obj_set_style_text_color(label, t.color_on_surface, 0);
        lv_obj_set_style_text_font(label, t.font_body_sm, 0);
    }

    return container;
}

void progress_set_value(lv_obj_t *progress, int32_t value)
{
    if (progress == nullptr) {
        return;
    }
    lv_obj_t *bar = bar_from(progress);
    if (bar == nullptr) {
        return;
    }
    lv_bar_set_value(bar, value, LV_ANIM_OFF);
    update_pct_label(bar, value, lv_bar_get_min_value(bar), lv_bar_get_max_value(bar));
}

int32_t progress_get_value(lv_obj_t *progress)
{
    if (progress == nullptr) {
        return 0;
    }
    lv_obj_t *bar = bar_from(progress);
    return bar != nullptr ? lv_bar_get_value(bar) : 0;
}

void progress_set_range(lv_obj_t *progress, int32_t min, int32_t max)
{
    if (progress == nullptr) {
        return;
    }
    lv_obj_t *bar = bar_from(progress);
    if (bar == nullptr) {
        return;
    }
    lv_bar_set_range(bar, min, max);
    update_pct_label(bar, lv_bar_get_value(bar), min, max);
}

void progress_set_label(lv_obj_t *progress, const char *text)
{
    if (progress == nullptr) {
        return;
    }
    lv_obj_t *label = label_from(progress);
    if (label != nullptr && text != nullptr) {
        lv_label_set_text(label, text);
    }
}

}  // namespace blusys
