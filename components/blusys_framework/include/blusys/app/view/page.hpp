#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "lvgl.h"

namespace blusys::app::view {

struct page_config {
    bool scrollable = false;
    int  padding    = -1;   // -1 = use theme().spacing_lg
    int  gap        = -1;   // -1 = use theme().spacing_md
};

struct page {
    lv_obj_t   *screen;    // root screen object
    lv_obj_t   *content;   // column container for adding children
    lv_group_t *group;     // encoder focus group
};

// Create a page: screen + column layout + encoder focus group.
// Does NOT load the screen — call page_load() when ready.
page page_create(const page_config &config = {});

// Load the page: calls lv_screen_load and wires encoder focus.
void page_load(page &p);

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
