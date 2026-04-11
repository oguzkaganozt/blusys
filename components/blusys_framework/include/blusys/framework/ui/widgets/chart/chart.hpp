#pragma once

#include <cstdint>

#include "lvgl.h"

// `bu_chart` — a themed line or bar chart.
//
// Visual intent: a data visualization surface for time-series or
// categorical data, supporting up to 4 concurrent series. Wraps
// `lv_chart` with theme styling.
//
// Display-only widget — no user interaction callbacks, no slot pool.
// Products add series, push data points, and call `chart_refresh` to
// trigger redraw after batch updates.

namespace blusys::ui {

enum class chart_type : uint8_t {
    line,
    bar,
};

struct chart_config {
    chart_type type        = chart_type::line;
    int32_t    point_count = 20;
    int32_t    y_min       = 0;
    int32_t    y_max       = 100;
    int32_t    series_max  = 4;
};

lv_obj_t *chart_create(lv_obj_t *parent, const chart_config &config);

int32_t chart_add_series(lv_obj_t *chart, lv_color_t color);
void    chart_set_next(lv_obj_t *chart, int32_t series, int32_t value);
void    chart_set_all(lv_obj_t *chart, int32_t series, const int32_t *values, int32_t count);
void    chart_set_range(lv_obj_t *chart, int32_t y_min, int32_t y_max);
void    chart_refresh(lv_obj_t *chart);

}  // namespace blusys::ui
