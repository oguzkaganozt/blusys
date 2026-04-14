#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/flows/connectivity_flow.hpp"
#include "blusys/framework/flows/diagnostics_flow.hpp"
#include "lvgl.h"

namespace blusys::screens {

struct status_screen_config {
    bool show_connectivity = true;
    bool show_diagnostics  = true;
    bool show_storage      = false;
    bool show_bluetooth    = false;
};

struct status_screen_handles {
    flows::connectivity_panel_handles connectivity;
    flows::diagnostics_panel_handles  diagnostics;
};

// screen_create_fn compatible — register with screen_router.
// Expects params to point to a status_screen_config.
lv_obj_t *status_screen_create(app_ctx &ctx, const void *params,
                                lv_group_t **group_out);

// Standalone factory that also populates widget handles for update calls.
lv_obj_t *status_screen_create(app_ctx &ctx, const status_screen_config &config,
                                status_screen_handles &handles,
                                lv_group_t **group_out);

// Update the status screen from current capability state.
void status_screen_update(status_screen_handles &handles, const app_ctx &ctx);

}  // namespace blusys::screens

#endif  // BLUSYS_FRAMEWORK_HAS_UI
