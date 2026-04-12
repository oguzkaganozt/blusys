#pragma once

// Shared LVGL flex helpers for stock widgets and primitives. Keeps layout
// behavior consistent: flex row strips vs content-sized children, etc.

#include "lvgl.h"

namespace blusys::ui::flex_layout {

/// True when `parent` uses a horizontal flex flow (row variants).
[[nodiscard]] inline bool parent_is_flex_row(lv_obj_t *parent)
{
    if (parent == nullptr) {
        return false;
    }
    const lv_flex_flow_t flow = lv_obj_get_style_flex_flow(parent, LV_PART_MAIN);
    return flow == LV_FLEX_FLOW_ROW || flow == LV_FLEX_FLOW_ROW_WRAP ||
           flow == LV_FLEX_FLOW_ROW_REVERSE || flow == LV_FLEX_FLOW_ROW_WRAP_REVERSE;
}

/// In a flex row, a child's `lv_obj_get_height` can track only its content
/// (e.g. after shrinking a sub-widget), while the row still has a larger inner
/// height. Use the row's content height as the effective cross-axis extent when
/// it exceeds the child's reported height.
[[nodiscard]] inline lv_coord_t effective_cross_extent_for_row_child(lv_obj_t *child)
{
    if (child == nullptr) {
        return 0;
    }
    lv_coord_t h = lv_obj_get_height(child);
    lv_obj_t  *parent = lv_obj_get_parent(child);
    if (parent != nullptr && parent_is_flex_row(parent)) {
        const lv_coord_t row_inner_h = lv_obj_get_content_height(parent);
        if (row_inner_h > h) {
            h = row_inner_h;
        }
    }
    return h;
}

/// True when `parent` uses a vertical flex flow (column variants).
[[nodiscard]] inline bool parent_is_flex_column(lv_obj_t *parent)
{
    if (parent == nullptr) {
        return false;
    }
    const lv_flex_flow_t flow = lv_obj_get_style_flex_flow(parent, LV_PART_MAIN);
    return flow == LV_FLEX_FLOW_COLUMN || flow == LV_FLEX_FLOW_COLUMN_WRAP ||
           flow == LV_FLEX_FLOW_COLUMN_REVERSE || flow == LV_FLEX_FLOW_COLUMN_WRAP_REVERSE;
}

/// In a flex column, a child's width can track only its content while the column
/// still has a larger inner width. Use the column's content width when larger.
[[nodiscard]] inline lv_coord_t effective_cross_extent_for_column_child(lv_obj_t *child)
{
    if (child == nullptr) {
        return 0;
    }
    lv_coord_t w = lv_obj_get_width(child);
    lv_obj_t  *parent = lv_obj_get_parent(child);
    if (parent != nullptr && parent_is_flex_column(parent)) {
        const lv_coord_t col_inner_w = lv_obj_get_content_width(parent);
        if (col_inner_w > w) {
            w = col_inner_w;
        }
    }
    return w;
}

}  // namespace blusys::ui::flex_layout
