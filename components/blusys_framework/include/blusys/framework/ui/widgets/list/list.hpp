#pragma once

#include <cstdint>

#include "blusys/framework/ui/callbacks.hpp"
#include "lvgl.h"

// `bu_list` — a scrollable, selectable item list.
//
// Visual intent: a vertical scrollable container of clickable rows,
// each optionally showing an icon glyph, primary text, and secondary
// text. Selection fires a `select_cb_t` with the item index.
//
// Products provide items as a `list_item` array at creation time and
// can replace them later via `list_set_items`. Programmatic selection
// via `list_set_selected` does not fire the callback.

namespace blusys::ui {

struct list_item {
    const char *text      = nullptr;
    const char *secondary = nullptr;   // optional second line
    const char *icon      = nullptr;   // optional icon glyph
};

struct list_config {
    const list_item *items      = nullptr;
    int32_t          item_count = 0;
    select_cb_t      on_select  = nullptr;
    void            *user_data  = nullptr;
    bool             disabled   = false;
};

lv_obj_t *list_create(lv_obj_t *parent, const list_config &config);

void    list_set_items(lv_obj_t *list, const list_item *items, int32_t count);
void    list_set_selected(lv_obj_t *list, int32_t index);
int32_t list_get_selected(lv_obj_t *list);
void    list_set_disabled(lv_obj_t *list, bool disabled);

}  // namespace blusys::ui
