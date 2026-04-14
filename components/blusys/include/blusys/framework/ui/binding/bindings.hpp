#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/primitives/status_badge.hpp"
#include "lvgl.h"

#include <cstdint>

namespace blusys {

// Simple state binding helpers.
//
// These are thin wrappers over LVGL and blusys::ui setter functions.
// The value is namespace consistency: app code stays in blusys::view
// and never touches LVGL or blusys::ui directly for common state changes.
//
// Not reactive — products call these from update() when state changes.

// Label text
void set_text(lv_obj_t *label, const char *text);

// Slider value (does not fire on_change callback)
void set_value(lv_obj_t *slider, std::int32_t value);
std::int32_t get_value(lv_obj_t *slider);

// Slider range
void set_range(lv_obj_t *slider, std::int32_t min, std::int32_t max);

// Toggle state (does not fire on_change callback)
void set_checked(lv_obj_t *toggle, bool on);
bool get_checked(lv_obj_t *toggle);

// Enabled/disabled (uses blusys::ui widget setters when available)
void set_enabled(lv_obj_t *widget, bool enabled);

// Visibility
void set_visible(lv_obj_t *widget, bool visible);

// ---- binding helpers ----

// Progress
void set_progress(lv_obj_t *progress, std::int32_t value);
std::int32_t get_progress(lv_obj_t *progress);

// List / dropdown / tabs selected index
void set_selected_index(lv_obj_t *widget, std::int32_t index);
std::int32_t get_selected_index(lv_obj_t *widget);

// Tabs active tab
void set_active_tab(lv_obj_t *tabs, std::int32_t index);

// Input field text
void set_input_text(lv_obj_t *input, const char *text);
const char *get_input_text(lv_obj_t *input);

// Status badge
void set_badge_text(lv_obj_t *badge, const char *text);
void set_badge_level(lv_obj_t *badge, blusys::badge_level level);

// Key-value
void set_kv_value(lv_obj_t *kv, const char *value);

// Data table cell
void set_cell_text(lv_obj_t *table, std::int32_t row, std::int32_t col, const char *text);

// Gauge / level bar / VU (display widgets — use from update() sync, not as owning handles)
void set_gauge_value(lv_obj_t *gauge, std::int32_t value);
void set_level_bar_value(lv_obj_t *bar, std::int32_t value);
void set_vu_strip_lit_segments(lv_obj_t *strip, std::uint8_t lit_segments);

// Composite: gauge + level bar + VU strip driven by one percent value (0–100).
struct percent_output_cluster {
    lv_obj_t    *gauge      = nullptr;
    lv_obj_t    *level_bar  = nullptr;
    lv_obj_t    *vu_strip   = nullptr;
    std::uint8_t vu_segment_count = 12;
};

void sync_percent_output(const percent_output_cluster &cluster, std::int32_t percent_0_100);

// Line chart: push a full series history and refresh (`blusys::ui` chart widget).
void sync_line_chart_series(lv_obj_t *chart,
                            std::int32_t series_index,
                            const std::int32_t *values,
                            std::int32_t count);

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
