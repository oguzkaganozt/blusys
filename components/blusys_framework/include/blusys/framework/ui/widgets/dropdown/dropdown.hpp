#pragma once

#include <cstdint>

#include "blusys/framework/ui/callbacks.hpp"
#include "lvgl.h"

// `bu_dropdown` — a themed dropdown selector.
//
// Visual intent: a compact button-like surface showing the currently
// selected option. On press, a popup list appears with all options.
// Selection fires a `select_cb_t` with the chosen index.
//
// Wraps `lv_dropdown` with theme styling, callback slot pool, and
// setter discipline.

namespace blusys::ui {

struct dropdown_config {
    const char * const *options      = nullptr;   // array of option strings
    int32_t             option_count = 0;
    int32_t             initial      = 0;         // initially selected index
    select_cb_t         on_select    = nullptr;
    void               *user_data    = nullptr;
    bool                disabled     = false;
};

lv_obj_t *dropdown_create(lv_obj_t *parent, const dropdown_config &config);

void    dropdown_set_selected(lv_obj_t *dropdown, int32_t index);
int32_t dropdown_get_selected(lv_obj_t *dropdown);
void    dropdown_set_options(lv_obj_t *dropdown, const char * const *options, int32_t count);
void    dropdown_set_disabled(lv_obj_t *dropdown, bool disabled);

}  // namespace blusys::ui
