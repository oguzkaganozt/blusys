#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/bindings.hpp"
#include "blusys/app/view/composites.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/framework/ui/primitives/status_badge.hpp"
#include "blusys/framework/ui/widgets/data_table/data_table.hpp"
#include "blusys/framework/ui/widgets/dropdown/dropdown.hpp"
#include "blusys/framework/ui/widgets/input_field/input_field.hpp"
#include "blusys/framework/ui/widgets/list/list.hpp"
#include "blusys/framework/ui/widgets/chart/chart.hpp"
#include "blusys/framework/ui/widgets/gauge/gauge.hpp"
#include "blusys/framework/ui/widgets/level_bar/level_bar.hpp"
#include "blusys/framework/ui/widgets/progress/progress.hpp"
#include "blusys/framework/ui/widgets/slider/slider.hpp"
#include "blusys/framework/ui/widgets/vu_strip/vu_strip.hpp"
#include "blusys/framework/ui/widgets/tabs/tabs.hpp"
#include "blusys/framework/ui/widgets/toggle/toggle.hpp"

namespace blusys::app::view {

void set_text(lv_obj_t *label, const char *text)
{
    if (label != nullptr && text != nullptr) {
        lv_label_set_text(label, text);
    }
}

void set_value(lv_obj_t *slider, std::int32_t value)
{
    if (slider != nullptr) {
        blusys::ui::slider_set_value(slider, value);
    }
}

std::int32_t get_value(lv_obj_t *slider)
{
    if (slider != nullptr) {
        return blusys::ui::slider_get_value(slider);
    }
    return 0;
}

void set_range(lv_obj_t *slider, std::int32_t min, std::int32_t max)
{
    if (slider != nullptr) {
        blusys::ui::slider_set_range(slider, min, max);
    }
}

void set_checked(lv_obj_t *toggle, bool on)
{
    if (toggle != nullptr) {
        blusys::ui::toggle_set_state(toggle, on);
    }
}

bool get_checked(lv_obj_t *toggle)
{
    if (toggle != nullptr) {
        return blusys::ui::toggle_get_state(toggle);
    }
    return false;
}

void set_enabled(lv_obj_t *widget, bool enabled)
{
    if (widget == nullptr) {
        return;
    }
    if (enabled) {
        lv_obj_clear_state(widget, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(widget, LV_STATE_DISABLED);
    }
}

void set_visible(lv_obj_t *widget, bool visible)
{
    if (widget == nullptr) {
        return;
    }
    if (visible) {
        lv_obj_clear_flag(widget, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(widget, LV_OBJ_FLAG_HIDDEN);
    }
}

// ---- binding helpers ----

void set_progress(lv_obj_t *progress, std::int32_t value)
{
    if (progress != nullptr) {
        blusys::ui::progress_set_value(progress, value);
    }
}

std::int32_t get_progress(lv_obj_t *progress)
{
    if (progress != nullptr) {
        return blusys::ui::progress_get_value(progress);
    }
    return 0;
}

void set_selected_index(lv_obj_t *widget, std::int32_t index)
{
    if (widget == nullptr) {
        return;
    }
    // Detect widget type by checking if it's an lv_dropdown (has dropdown API).
    // lv_dropdown objects have LV_OBJ_CLASS_INHERITS_FROM(lv_dropdown_class).
    if (lv_obj_check_type(widget, &lv_dropdown_class)) {
        blusys::ui::dropdown_set_selected(widget, index);
    } else {
        blusys::ui::list_set_selected(widget, index);
    }
}

std::int32_t get_selected_index(lv_obj_t *widget)
{
    if (widget == nullptr) {
        return -1;
    }
    if (lv_obj_check_type(widget, &lv_dropdown_class)) {
        return blusys::ui::dropdown_get_selected(widget);
    }
    return blusys::ui::list_get_selected(widget);
}

void set_active_tab(lv_obj_t *tabs, std::int32_t index)
{
    if (tabs != nullptr) {
        blusys::ui::tabs_set_active(tabs, index);
    }
}

void set_input_text(lv_obj_t *input, const char *text)
{
    if (input != nullptr) {
        blusys::ui::input_field_set_text(input, text);
    }
}

const char *get_input_text(lv_obj_t *input)
{
    if (input != nullptr) {
        return blusys::ui::input_field_get_text(input);
    }
    return "";
}

void set_badge_text(lv_obj_t *badge, const char *text)
{
    if (badge != nullptr) {
        blusys::ui::status_badge_set_text(badge, text);
    }
}

void set_badge_level(lv_obj_t *badge, blusys::ui::badge_level level)
{
    if (badge != nullptr) {
        blusys::ui::status_badge_set_level(badge, level);
    }
}

void set_kv_value(lv_obj_t *kv, const char *value)
{
    if (kv != nullptr) {
        blusys::ui::key_value_set_value(kv, value);
    }
}

void set_cell_text(lv_obj_t *table, std::int32_t row, std::int32_t col, const char *text)
{
    if (table != nullptr) {
        blusys::ui::data_table_set_cell(table, row, col, text);
    }
}

void set_gauge_value(lv_obj_t *gauge, std::int32_t value)
{
    if (gauge != nullptr) {
        blusys::ui::gauge_set_value(gauge, value);
    }
}

void set_level_bar_value(lv_obj_t *bar, std::int32_t value)
{
    if (bar != nullptr) {
        blusys::ui::level_bar_set_value(bar, value);
    }
}

void set_vu_strip_lit_segments(lv_obj_t *strip, std::uint8_t lit_segments)
{
    if (strip != nullptr) {
        blusys::ui::vu_strip_set_value(strip, lit_segments);
    }
}

void sync_percent_output(const percent_output_cluster &cluster, std::int32_t percent_0_100)
{
    if (percent_0_100 < 0) {
        percent_0_100 = 0;
    } else if (percent_0_100 > 100) {
        percent_0_100 = 100;
    }
    set_gauge_value(cluster.gauge, percent_0_100);
    set_level_bar_value(cluster.level_bar, percent_0_100);
    if (cluster.vu_strip != nullptr && cluster.vu_segment_count > 0) {
        const auto lit = static_cast<std::uint8_t>(
            (percent_0_100 * static_cast<std::int32_t>(cluster.vu_segment_count)) / 100);
        set_vu_strip_lit_segments(cluster.vu_strip, lit);
    }
}

void sync_labeled_value(const labeled_value_row &row, const char *value_text)
{
    if (row.value != nullptr) {
        set_text(row.value, value_text != nullptr ? value_text : "");
    }
}

void sync_labeled_value(const labeled_value_row &row, const char *caption_text, const char *value_text)
{
    if (row.caption != nullptr) {
        set_text(row.caption, caption_text != nullptr ? caption_text : "");
    }
    sync_labeled_value(row, value_text);
}

void sync_status_strip(const status_strip &strip,
                       blusys::ui::badge_level level,
                       const char *badge_text,
                       const char *detail_text)
{
    set_badge_level(strip.badge, level);
    set_badge_text(strip.badge, badge_text);
    if (strip.detail_label != nullptr) {
        set_text(strip.detail_label, detail_text != nullptr ? detail_text : "");
    }
}

void sync_key_value_quad(const key_value_quad &row,
                         const char *v0,
                         const char *v1,
                         const char *v2,
                         const char *v3)
{
    const char *vals[] = {v0, v1, v2, v3};
    for (int i = 0; i < 4; ++i) {
        if (row.kv[i] != nullptr && vals[i] != nullptr) {
            set_kv_value(row.kv[i], vals[i]);
        }
    }
}

void sync_key_value_trio(const key_value_trio &row,
                         const char *v0,
                         const char *v1,
                         const char *v2)
{
    const char *vals[] = {v0, v1, v2};
    for (int i = 0; i < 3; ++i) {
        if (row.kv[i] != nullptr && vals[i] != nullptr) {
            set_kv_value(row.kv[i], vals[i]);
        }
    }
}

void sync_line_chart_series(lv_obj_t *chart,
                            std::int32_t series_index,
                            const std::int32_t *values,
                            std::int32_t count)
{
    if (chart == nullptr || values == nullptr || count <= 0 || series_index < 0) {
        return;
    }
    blusys::ui::chart_set_all(chart, series_index, values, count);
    blusys::ui::chart_refresh(chart);
}

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
