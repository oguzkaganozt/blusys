#pragma once

#include <cstdint>

#include "blusys/framework/callbacks.hpp"
#include "lvgl.h"

// `bu_data_table` — a themed tabular data display.
//
// Visual intent: a header row followed by data rows with alternating
// backgrounds. Optionally supports row selection via `select_cb_t`.
//
// Wraps `lv_table` with theme styling, callback slot pool, and setter
// discipline. Products populate cells through `data_table_set_cell`.

namespace blusys {

struct table_column {
    const char *header = nullptr;
    int32_t     width  = 0;   // 0 = auto
};

struct data_table_config {
    const table_column *columns       = nullptr;
    int32_t             col_count     = 0;
    int32_t             row_count     = 0;
    select_cb_t         on_row_select = nullptr;
    void               *user_data     = nullptr;
};

lv_obj_t *data_table_create(lv_obj_t *parent, const data_table_config &config);

void        data_table_set_cell(lv_obj_t *table, int32_t row, int32_t col, const char *text);
const char *data_table_get_cell(lv_obj_t *table, int32_t row, int32_t col);
void        data_table_set_row_count(lv_obj_t *table, int32_t count);
void        data_table_set_selected_row(lv_obj_t *table, int32_t row);

}  // namespace blusys
