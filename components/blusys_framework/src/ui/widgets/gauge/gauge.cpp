#include "blusys/framework/ui/widgets/gauge/gauge.hpp"

#include <cstdio>

#include "blusys/framework/ui/detail/fixed_slot_pool.hpp"
#include "blusys/framework/ui/detail/flex_layout.hpp"
#include "blusys/framework/ui/detail/widget_common.hpp"
#include "blusys/framework/ui/theme.hpp"

// Display-only arc gauge. No callbacks; fixed-capacity data pool via fixed_slot_pool.
//
// Internal layout: an `lv_arc` in indicator-only mode (knob part removed)
// with an optional centered label showing the current value and unit.
// The arc user_data stores a pointer to the center label (or nullptr).
//
// The `unit` string pointer is stored in a small struct attached to the
// outer container's user_data to support `gauge_set_unit`.

namespace blusys::ui {
namespace {

#ifndef BLUSYS_UI_GAUGE_POOL_SIZE
#define BLUSYS_UI_GAUGE_POOL_SIZE 16
#endif

struct gauge_data {
    lv_obj_t   *arc;
    lv_obj_t   *label;
    const char *unit;
    int32_t     min;
    int32_t     max;
    bool        in_use;
};

void apply_arc_theme(lv_obj_t *arc)
{
    const auto &t = theme();

    // Background arc (track).
    lv_obj_set_style_arc_color(arc, t.color_disabled, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_MAIN);

    // Indicator arc (filled portion).
    lv_obj_set_style_arc_color(arc, t.color_primary, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);

    // Hide knob.
    lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
}

void update_label(gauge_data *data, int32_t value)
{
    if (data->label == nullptr) {
        return;
    }
    char buf[32];
    if (data->unit != nullptr) {
        std::snprintf(buf, sizeof(buf), "%d%s", static_cast<int>(value), data->unit);
    } else {
        std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(value));
    }
    lv_label_set_text(data->label, buf);
}

// Display-only: gauges are typically few per screen, kept in a fixed pool.
detail::slot_pool<gauge_data, BLUSYS_UI_GAUGE_POOL_SIZE> g_gauge_pool{
    "ui.gauge", "BLUSYS_UI_GAUGE_POOL_SIZE"};

// Scale the arc to a square that fits inside the container. Flex layouts often
// assign a smaller height than the arc's default size; without this the arc
// draws past the clip and the ring looks cut off at the top/bottom.
//
// In a flex *row* with cross-axis center, the gauge container's height tracks
// its content (the arc). After we shrink the arc, that height collapses to the
// tiny arc — use the parent row's inner height so the ring fills the strip
// (see flex_layout::effective_cross_extent_for_row_child).
void sync_arc_size_to_container(lv_obj_t *container)
{
    auto *data = static_cast<gauge_data *>(lv_obj_get_user_data(container));
    if (data == nullptr || data->arc == nullptr) {
        return;
    }

    const lv_coord_t w = lv_obj_get_width(container);
    const lv_coord_t h = flex_layout::effective_cross_extent_for_row_child(container);

    if (w < 8 || h < 8) {
        return;
    }

    constexpr lv_coord_t kInset = 4;
    lv_coord_t           s      = w < h ? w : h;
    s -= kInset;
    if (s < 32) {
        return;
    }

    // Early-out when the arc is already the target size. The container uses
    // LV_SIZE_CONTENT, so resizing the arc can re-fire LV_EVENT_SIZE_CHANGED
    // on the container — this breaks the feedback loop.
    if (lv_obj_get_width(data->arc) == s && lv_obj_get_height(data->arc) == s) {
        return;
    }

    lv_obj_set_size(data->arc, s, s);
    lv_obj_center(data->arc);
    if (data->label != nullptr) {
        lv_obj_center(data->label);
    }
}

void on_gauge_container_size(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED) {
        return;
    }
    sync_arc_size_to_container(static_cast<lv_obj_t *>(lv_event_get_target(e)));
}

}  // namespace

lv_obj_t *gauge_create(lv_obj_t *parent, const gauge_config &config)
{
    gauge_data *data = g_gauge_pool.acquire();
    if (data == nullptr) {
        return nullptr;
    }

    const auto &t = theme();

    // Container to hold arc and label.
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(container, data);
    lv_obj_add_event_cb(container, detail::release_slot_on_delete<gauge_data>,
                        LV_EVENT_DELETE, nullptr);
    lv_obj_add_event_cb(container, on_gauge_container_size, LV_EVENT_SIZE_CHANGED, nullptr);

    // Arc.
    lv_obj_t *arc = lv_arc_create(container);
    apply_arc_theme(arc);
    lv_arc_set_rotation(arc, static_cast<uint16_t>(config.start_angle));
    lv_arc_set_bg_angles(arc, 0,
                         static_cast<uint16_t>(config.end_angle - config.start_angle));
    lv_arc_set_range(arc, config.min, config.max);
    lv_arc_set_value(arc, config.initial);

    // Remove interactivity — gauge is display-only.
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);

    data->arc  = arc;
    data->unit = config.unit;
    data->min  = config.min;
    data->max  = config.max;

    // Optional center value label.
    if (config.show_value) {
        lv_obj_t *label = lv_label_create(container);
        lv_obj_set_style_text_font(label, t.font_display, 0);
        lv_obj_set_style_text_color(label, t.color_on_surface, 0);
        lv_obj_center(label);
        data->label = label;
        update_label(data, config.initial);
    } else {
        data->label = nullptr;
    }

    return container;
}

void gauge_set_value(lv_obj_t *gauge, int32_t value)
{
    if (gauge == nullptr) {
        return;
    }
    auto *data = static_cast<gauge_data *>(lv_obj_get_user_data(gauge));
    if (data == nullptr || data->arc == nullptr) {
        return;
    }
    lv_arc_set_value(data->arc, value);
    update_label(data, value);
}

int32_t gauge_get_value(lv_obj_t *gauge)
{
    if (gauge == nullptr) {
        return 0;
    }
    auto *data = static_cast<gauge_data *>(lv_obj_get_user_data(gauge));
    if (data == nullptr || data->arc == nullptr) {
        return 0;
    }
    return lv_arc_get_value(data->arc);
}

void gauge_set_range(lv_obj_t *gauge, int32_t min, int32_t max)
{
    if (gauge == nullptr) {
        return;
    }
    auto *data = static_cast<gauge_data *>(lv_obj_get_user_data(gauge));
    if (data == nullptr || data->arc == nullptr) {
        return;
    }
    data->min = min;
    data->max = max;
    lv_arc_set_range(data->arc, min, max);
}

void gauge_set_unit(lv_obj_t *gauge, const char *unit)
{
    if (gauge == nullptr) {
        return;
    }
    auto *data = static_cast<gauge_data *>(lv_obj_get_user_data(gauge));
    if (data == nullptr) {
        return;
    }
    data->unit = unit;
    update_label(data, lv_arc_get_value(data->arc));
}

}  // namespace blusys::ui
