#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/capabilities/connectivity.hpp"
#include "lvgl.h"

namespace blusys::app {
class app_ctx;
}

namespace blusys::app::flows {

struct connectivity_display_config {
    bool show_ip       = true;
    bool show_services = true;
};

struct connectivity_panel_handles {
    lv_obj_t *wifi_badge  = nullptr;
    lv_obj_t *ip_label    = nullptr;
    lv_obj_t *sntp_badge  = nullptr;
    lv_obj_t *mdns_badge  = nullptr;
    lv_obj_t *ctrl_badge  = nullptr;
};

// Create a connectivity status panel as a child of parent.
// Not a standalone screen — embeddable in status/settings/about screens.
// If handles is non-null, widget pointers are stored for later update calls.
lv_obj_t *connectivity_panel_create(lv_obj_t *parent,
                                     connectivity_panel_handles *handles = nullptr,
                                     const connectivity_display_config &config = {});

// Update the panel widgets from the current connectivity_status.
void connectivity_panel_update(connectivity_panel_handles &handles,
                                const connectivity_status &status);

// Primary action wired to `app_ctx::request_connectivity_reconnect()`.
// Uses a single static bridge — only one reconnect control per screen tree.
lv_obj_t *connectivity_reconnect_button_create(lv_obj_t *parent, blusys::app::app_ctx &ctx,
                                                const char *label = "Reconnect");

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
