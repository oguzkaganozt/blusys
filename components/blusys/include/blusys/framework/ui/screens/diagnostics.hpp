#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/flows/diagnostics_flow.hpp"
#include "blusys/framework/callbacks.hpp"
#include "lvgl.h"

namespace blusys::screens {

struct diagnostics_screen_config {
    bool show_restart_button = true;
    bool show_reset_prov     = false;
    blusys::press_cb_t on_restart = nullptr;
    void *restart_ctx                  = nullptr;
};

// screen_create_fn compatible — register with screen_router.
// Expects params to point to a diagnostics_screen_config.
lv_obj_t *diagnostics_screen_create(app_ctx &ctx, const void *params,
                                     lv_group_t **group_out);

}  // namespace blusys::screens

#endif  // BLUSYS_FRAMEWORK_HAS_UI
