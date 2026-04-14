#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "lvgl.h"

namespace blusys {

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

// Create a page inside an existing parent (e.g., a shell content area).
// The page does NOT own its own screen — it lives inside the parent.
// The returned page.screen points to the parent for compatibility.
//
// In the interaction shell, `parent` should be `shell.content_area` (a flex
// column). The content column uses flex_grow + min_height(0) and optional
// scroll so the viewport stays bounded — see docs/internals/architecture.md#ui-layout-lvgl-flex.
page page_create_in(lv_obj_t *parent, const page_config &config = {});

// Load the page: calls lv_screen_load and wires encoder focus.
void page_load(page &p);

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
