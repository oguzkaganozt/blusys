#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/callbacks.hpp"
#include "lvgl.h"

namespace blusys::flows {

struct loading_config {
    const char *message      = "Loading...";
    bool        show_spinner = true;
};

// Create a loading indicator as a child of parent.
// Not a standalone screen — products embed this in their own screens.
lv_obj_t *loading_create(lv_obj_t *parent, const loading_config &config = {});

struct empty_state_config {
    const char *title   = "No Data";
    const char *message = nullptr;
    // Optional primary action (e.g. “Retry” / “Refresh”) — uses semantic press callback.
    const char           *primary_label = nullptr;
    blusys::press_cb_t  on_primary  = nullptr;
    void                   *primary_user_data = nullptr;
};

// Create an empty-state placeholder as a child of parent.
lv_obj_t *empty_state_create(lv_obj_t *parent, const empty_state_config &config = {});

}  // namespace blusys::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
