#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/flows/flow_base.hpp"
#include "blusys/framework/ui/composition/screen_registry.hpp"
#include "lvgl.h"

namespace blusys::flows {

struct provisioning_flow_config {
    const char *title       = "Device Setup";
    const char *qr_label    = "Scan to connect:";
    const char *waiting_msg = "Waiting for credentials...";
    const char *success_msg = "Connected successfully!";
    const char *error_msg   = "Setup failed. Try again.";
};

struct provisioning_screen_handles {
    lv_obj_t *status_label = nullptr;
    lv_obj_t *qr_label     = nullptr;
    lv_obj_t *action_btn   = nullptr;
};

// screen_create_fn compatible — register with screen_router.
// Expects params to point to a provisioning_flow_config.
lv_obj_t *provisioning_screen_create(app_ctx &ctx, const void *params,
                                      lv_group_t **group_out);

// Standalone factory that also populates widget handles for update calls.
lv_obj_t *provisioning_screen_create(app_ctx &ctx,
                                      const provisioning_flow_config &config,
                                      provisioning_screen_handles &handles,
                                      lv_group_t **group_out);

// Update the provisioning screen widgets from the current connectivity-owned
// provisioning status.
void provisioning_screen_update(provisioning_screen_handles &handles,
                                  const connectivity_provisioning_status &status,
                                  const provisioning_flow_config &config = {});

// provisioning_flow — registers a provisioning screen via spec.flows.
//
// The create_fn receives app_ctx at render time and should call
// provisioning_screen_create() with app-specific config and handles.
// Handles (provisioning_screen_handles) are owned by the UI layer
// (e.g., a module-scope static in app_ui.cpp), not by app_state.
//
// Usage:
//   static blusys::flows::provisioning_flow kProv = []() {
//       blusys::flows::provisioning_flow f;
//       f.route_id         = static_cast<uint32_t>(route::setup);
//       f.create_fn        = &my_create_setup;
//       f.lifecycle.on_show = on_show_cb;
//       f.lifecycle.on_hide = on_hide_cb;
//       return f;
//   }();
//   // In app_main.cpp spec: .flows = {&kProv}, .flow_count = 1
class provisioning_flow : public flow_base {
public:
    std::uint32_t    route_id  = 0;
    screen_create_fn create_fn = nullptr;
    screen_lifecycle lifecycle  = {};

    void start(blusys::app_ctx &ctx) override
    {
        if (create_fn != nullptr) {
            screen_lifecycle lc = lifecycle;
            if (lc.user_data == nullptr) lc.user_data = &ctx;
            ctx.fx().nav.register_screen(route_id, create_fn, nullptr, lc);
        }
    }
};

}  // namespace blusys::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
