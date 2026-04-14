#pragma once

#include <cstdint>

#include "blusys/framework/callbacks.hpp"
#include "lvgl.h"

// `bu_tabs` — a content-level tab switcher widget.
//
// Visual intent: a horizontal strip of tab labels above a content area.
// Clicking a tab activates it and reveals its content panel. Fires
// `select_cb_t` with the new tab index on change.
//
// Distinct from the shell tab_bar (which is a navigation bar). This
// widget manages local content switching within a screen or section.
//
// Products populate each tab's content by calling `tabs_get_content()`
// and adding children to the returned container.

namespace blusys {

static constexpr int kMaxContentTabs = 8;

struct tab_item {
    const char *label = nullptr;
};

struct tabs_config {
    const tab_item *items      = nullptr;
    int32_t         item_count = 0;
    int32_t         initial    = 0;
    select_cb_t     on_change  = nullptr;
    void           *user_data  = nullptr;
};

lv_obj_t *tabs_create(lv_obj_t *parent, const tabs_config &config);

void      tabs_set_active(lv_obj_t *tabs, int32_t index);
int32_t   tabs_get_active(lv_obj_t *tabs);
lv_obj_t *tabs_get_content(lv_obj_t *tabs, int32_t index);

}  // namespace blusys
