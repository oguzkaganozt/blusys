#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/capabilities/ota.hpp"
#include "lvgl.h"

namespace blusys::flows {

struct ota_flow_config {
    const char *title           = "Firmware Update";
    const char *checking_msg    = "Checking for updates...";
    const char *downloading_msg = "Downloading...";
    const char *applying_msg    = "Applying update...";
    const char *success_msg     = "Update complete. Restarting...";
    const char *error_msg       = "Update failed.";
};

struct ota_screen_handles {
    lv_obj_t *progress_bar  = nullptr;
    lv_obj_t *status_label  = nullptr;
    lv_obj_t *action_btn    = nullptr;
};

// screen_create_fn compatible — register with screen_router.
// Expects params to point to an ota_flow_config.
lv_obj_t *ota_screen_create(app_ctx &ctx, const void *params,
                             lv_group_t **group_out);

// Standalone factory that also populates widget handles for update calls.
lv_obj_t *ota_screen_create(app_ctx &ctx, const ota_flow_config &config,
                             ota_screen_handles &handles,
                             lv_group_t **group_out);

// Update the OTA screen widgets from the current ota_status.
void ota_screen_update(ota_screen_handles &handles,
                        const ota_status &status,
                        const ota_flow_config &config = {});

}  // namespace blusys::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
