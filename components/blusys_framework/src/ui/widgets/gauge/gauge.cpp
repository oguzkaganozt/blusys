#include "blusys/framework/ui/widgets/gauge/gauge.hpp"

#include <cstdio>

#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Display-only arc gauge. No callbacks, no slot pool.
//
// Internal layout: an `lv_arc` in indicator-only mode (knob part removed)
// with an optional centered label showing the current value and unit.
// The arc user_data stores a pointer to the center label (or nullptr).
//
// The `unit` string pointer is stored in a small struct attached to the
// outer container's user_data to support `gauge_set_unit`.

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.gauge";

struct gauge_data {
    lv_obj_t   *arc;
    lv_obj_t   *label;
    const char *unit;
    int32_t     min;
    int32_t     max;
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

// We store gauge_data in a small static pool to avoid dynamic allocation.
// Gauges are display-only and typically few per screen.
static constexpr int kGaugeDataPoolSize = 16;
gauge_data g_gauge_data[kGaugeDataPoolSize] = {};
bool       g_gauge_data_used[kGaugeDataPoolSize] = {};

gauge_data *acquire_data()
{
    for (int i = 0; i < kGaugeDataPoolSize; ++i) {
        if (!g_gauge_data_used[i]) {
            g_gauge_data_used[i] = true;
            return &g_gauge_data[i];
        }
    }
    BLUSYS_LOGE(kTag, "data pool exhausted (size=%d)", kGaugeDataPoolSize);
    return nullptr;
}

void release_data(gauge_data *d)
{
    for (int i = 0; i < kGaugeDataPoolSize; ++i) {
        if (&g_gauge_data[i] == d) {
            g_gauge_data_used[i] = false;
            return;
        }
    }
}

void on_gauge_deleted(lv_event_t *e)
{
    auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *d   = static_cast<gauge_data *>(lv_obj_get_user_data(obj));
    release_data(d);
}

}  // namespace

lv_obj_t *gauge_create(lv_obj_t *parent, const gauge_config &config)
{
    gauge_data *data = acquire_data();
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
    lv_obj_add_event_cb(container, on_gauge_deleted, LV_EVENT_DELETE, nullptr);

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
