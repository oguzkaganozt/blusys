#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/app_ctx.hpp"
#include "blusys/app/flows/diagnostics_flow.hpp"
#include "blusys/framework/ui/callbacks.hpp"
#include "lvgl.h"

namespace blusys::app::screens {

struct diagnostics_screen_config {
    bool show_restart_button = true;
    bool show_reset_prov     = false;
    blusys::ui::press_cb_t on_restart = nullptr;
    void *restart_ctx                  = nullptr;
};

// screen_create_fn compatible — register with screen_router.
// Expects params to point to a diagnostics_screen_config.
lv_obj_t *diagnostics_screen_create(app_ctx &ctx, const void *params,
                                     lv_group_t **group_out);

}  // namespace blusys::app::screens

#endif  // BLUSYS_FRAMEWORK_HAS_UI
