#include "blusys/framework/ui/widgets/chart/chart.hpp"

#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Display-only chart wrapping `lv_chart`. No callbacks, no slot pool.
//
// Products add series with `chart_add_series` and push data points with
// `chart_set_next` or `chart_set_all`. Series pointers are tracked in a
// small static pool attached via user_data.

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.chart";
static constexpr int kMaxSeries = 4;

struct chart_data {
    lv_chart_series_t *series[kMaxSeries];
    int32_t            series_count;
    int32_t            series_max;
    bool               in_use;
};

static constexpr int kChartDataPoolSize = 8;
chart_data g_chart_data[kChartDataPoolSize] = {};

chart_data *acquire_data()
{
    for (auto &d : g_chart_data) {
        if (!d.in_use) {
            d.in_use = true;
            d.series_count = 0;
            for (auto &s : d.series) { s = nullptr; }
            return &d;
        }
    }
    BLUSYS_LOGE(kTag, "data pool exhausted (size=%d)", kChartDataPoolSize);
    return nullptr;
}

void release_data(chart_data *d)
{
    if (d != nullptr) {
        d->in_use = false;
    }
}

void on_chart_deleted(lv_event_t *e)
{
    auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *d   = static_cast<chart_data *>(lv_obj_get_user_data(obj));
    release_data(d);
}

}  // namespace

lv_obj_t *chart_create(lv_obj_t *parent, const chart_config &config)
{
    chart_data *data = acquire_data();
    if (data == nullptr) {
        return nullptr;
    }
    data->series_max = config.series_max > kMaxSeries ? kMaxSeries : config.series_max;

    const auto &t = theme();

    lv_obj_t *chart = lv_chart_create(parent);

    // Chart type.
    lv_chart_type_t lv_type = (config.type == chart_type::bar)
                                  ? LV_CHART_TYPE_BAR
                                  : LV_CHART_TYPE_LINE;
    lv_chart_set_type(chart, lv_type);
    lv_chart_set_point_count(chart, static_cast<uint32_t>(config.point_count));
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, config.y_min, config.y_max);

    // Theme styling.
    lv_obj_set_style_bg_color(chart, t.color_surface_elevated, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(chart, t.radius_card, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, t.spacing_sm, LV_PART_MAIN);

    // Grid / division lines.
    lv_obj_set_style_line_color(chart, t.color_outline_variant, LV_PART_MAIN);
    lv_chart_set_div_line_count(chart, 5, 3);

    // Series point styling.
    lv_obj_set_style_size(chart, 4, 4, LV_PART_INDICATOR);

    lv_obj_set_width(chart, LV_PCT(100));
    lv_obj_set_height(chart, LV_PCT(100));

    lv_obj_set_user_data(chart, data);
    lv_obj_add_event_cb(chart, on_chart_deleted, LV_EVENT_DELETE, nullptr);

    return chart;
}

int32_t chart_add_series(lv_obj_t *chart, lv_color_t color)
{
    if (chart == nullptr) {
        return -1;
    }
    auto *data = static_cast<chart_data *>(lv_obj_get_user_data(chart));
    if (data == nullptr || data->series_count >= data->series_max) {
        return -1;
    }

    lv_chart_series_t *ser = lv_chart_add_series(chart, color, LV_CHART_AXIS_PRIMARY_Y);
    if (ser == nullptr) {
        return -1;
    }
    int32_t idx = data->series_count;
    data->series[idx] = ser;
    data->series_count++;
    return idx;
}

void chart_set_next(lv_obj_t *chart, int32_t series, int32_t value)
{
    if (chart == nullptr) {
        return;
    }
    auto *data = static_cast<chart_data *>(lv_obj_get_user_data(chart));
    if (data == nullptr || series < 0 || series >= data->series_count) {
        return;
    }
    lv_chart_set_next_value(chart, data->series[series], value);
}

void chart_set_all(lv_obj_t *chart, int32_t series, const int32_t *values, int32_t count)
{
    if (chart == nullptr || values == nullptr) {
        return;
    }
    auto *data = static_cast<chart_data *>(lv_obj_get_user_data(chart));
    if (data == nullptr || series < 0 || series >= data->series_count) {
        return;
    }
    for (int32_t i = 0; i < count; ++i) {
        lv_chart_set_next_value(chart, data->series[series], values[i]);
    }
}

void chart_set_range(lv_obj_t *chart, int32_t y_min, int32_t y_max)
{
    if (chart == nullptr) {
        return;
    }
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
}

void chart_refresh(lv_obj_t *chart)
{
    if (chart == nullptr) {
        return;
    }
    lv_chart_refresh(chart);
}

}  // namespace blusys::ui
