#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/app_ctx.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "lvgl.h"

namespace blusys::app::flows {

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

// Update the provisioning screen widgets from the current provisioning_status.
void provisioning_screen_update(provisioning_screen_handles &handles,
                                 const provisioning_status &status,
                                 const provisioning_flow_config &config = {});

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
