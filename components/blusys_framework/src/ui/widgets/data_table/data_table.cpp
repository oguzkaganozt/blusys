#include "blusys/framework/ui/widgets/data_table/data_table.hpp"

#include "blusys/framework/ui/detail/fixed_slot_pool.hpp"
#include "blusys/framework/ui/detail/widget_common.hpp"
#include "blusys/framework/ui/theme.hpp"

// Themed tabular data wrapping `lv_table`.
//
// Row 0 is the header row (styled with font_label). Data rows start at
// index 1. `LV_EVENT_VALUE_CHANGED` fires on cell click — we extract
// the row and fire `select_cb_t(data_row_index, user_data)`.
//
// Setter discipline: `data_table_set_selected_row` does not fire callback.

#ifndef BLUSYS_UI_DATA_TABLE_POOL_SIZE
#define BLUSYS_UI_DATA_TABLE_POOL_SIZE 8
#endif

namespace blusys::ui {
namespace {

struct data_table_slot {
    select_cb_t  on_row_select;
    void        *user_data;
    int32_t      col_count;
    bool         in_use;
};

detail::slot_pool<data_table_slot, BLUSYS_UI_DATA_TABLE_POOL_SIZE> g_data_table_pool{
    "ui.data_table", "BLUSYS_UI_DATA_TABLE_POOL_SIZE"};

void apply_theme(lv_obj_t *table)
{
    const auto &t = theme();

    // Table background.
    lv_obj_set_style_bg_color(table, t.color_surface_elevated, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(table, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(table, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(table, t.radius_card, LV_PART_MAIN);

    // Cell styling.
    lv_obj_set_style_border_color(table, t.color_outline_variant, LV_PART_ITEMS);
    lv_obj_set_style_border_width(table, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_side(table, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS);
    lv_obj_set_style_text_color(table, t.color_on_surface, LV_PART_ITEMS);
    lv_obj_set_style_text_font(table, t.font_mono, LV_PART_ITEMS);
    lv_obj_set_style_pad_left(table, t.spacing_sm, LV_PART_ITEMS);
    lv_obj_set_style_pad_right(table, t.spacing_sm, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(table, t.spacing_xs, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(table, t.spacing_xs, LV_PART_ITEMS);
}

void on_lvgl_value_changed(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<data_table_slot *>(lv_obj_get_user_data(obj));
    if (slot == nullptr || slot->on_row_select == nullptr) {
        return;
    }

    uint32_t row = 0;
    uint32_t col = 0;
    lv_table_get_selected_cell(obj, &row, &col);

    // Row 0 is the header — data rows start at 1.
    if (row == 0 || row == LV_TABLE_CELL_NONE) {
        return;
    }

    int32_t data_row = static_cast<int32_t>(row) - 1;
    slot->on_row_select(data_row, slot->user_data);
}

}  // namespace

lv_obj_t *data_table_create(lv_obj_t *parent, const data_table_config &config)
{
    data_table_slot *slot = nullptr;
    if (config.on_row_select != nullptr) {
        slot = g_data_table_pool.acquire();
        if (slot == nullptr) {
            return nullptr;
        }
        slot->on_row_select = config.on_row_select;
        slot->user_data     = config.user_data;
        slot->col_count     = config.col_count;
    }

    lv_obj_t *table = lv_table_create(parent);
    apply_theme(table);

    // Set column count.
    lv_table_set_column_count(table, static_cast<uint32_t>(config.col_count));

    // Set total rows: 1 header + data rows.
    lv_table_set_row_count(table, static_cast<uint32_t>(config.row_count + 1));

    // Configure columns.
    for (int32_t c = 0; c < config.col_count; ++c) {
        if (config.columns != nullptr) {
            // Header row.
            lv_table_set_cell_value(table, 0, static_cast<uint32_t>(c),
                                    config.columns[c].header != nullptr
                                        ? config.columns[c].header : "");
            // Column width.
            if (config.columns[c].width > 0) {
                lv_table_set_column_width(table, static_cast<uint32_t>(c),
                                          static_cast<int32_t>(config.columns[c].width));
            }
        }
    }

    lv_obj_set_width(table, LV_PCT(100));

    if (slot != nullptr) {
        lv_obj_set_user_data(table, slot);
        lv_obj_add_event_cb(table, on_lvgl_value_changed, LV_EVENT_VALUE_CHANGED, nullptr);
        lv_obj_add_event_cb(table, detail::release_slot_on_delete<data_table_slot>,
                            LV_EVENT_DELETE, nullptr);
    }

    return table;
}

void data_table_set_cell(lv_obj_t *table, int32_t row, int32_t col, const char *text)
{
    if (table == nullptr) {
        return;
    }
    // Data row offset: row 0 in API = row 1 in lv_table (row 0 is header).
    lv_table_set_cell_value(table, static_cast<uint32_t>(row + 1),
                            static_cast<uint32_t>(col),
                            text != nullptr ? text : "");
}

const char *data_table_get_cell(lv_obj_t *table, int32_t row, int32_t col)
{
    if (table == nullptr) {
        return "";
    }
    return lv_table_get_cell_value(table, static_cast<uint32_t>(row + 1),
                                   static_cast<uint32_t>(col));
}

void data_table_set_row_count(lv_obj_t *table, int32_t count)
{
    if (table == nullptr) {
        return;
    }
    // +1 for header row.
    lv_table_set_row_count(table, static_cast<uint32_t>(count + 1));
}

void data_table_set_selected_row(lv_obj_t *table, int32_t row)
{
    if (table == nullptr) {
        return;
    }
    // Visual highlight — set the selected cell to the first column of the row.
    // This is a programmatic setter so no callback fires.
    (void)row;
    // lv_table does not have a built-in "selected row" highlight API.
    // For now this is a no-op placeholder. Full highlight requires custom
    // draw event handling which can be added as a follow-up enhancement.
}

}  // namespace blusys::ui
