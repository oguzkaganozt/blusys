#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/app/ctx.hpp"
#include "lvgl.h"

namespace blusys::flows {

struct boot_splash_config {
    const char *product_name = nullptr;
    const char *version      = nullptr;
    const char *tagline      = nullptr;
    std::uint32_t display_ms = 2000;
    // Called when the splash auto-dismisses. Use this to navigate to
    // the first real screen.
    void (*on_complete)(app_ctx &ctx) = nullptr;
};

// screen_create_fn compatible — register with screen_router.
// Expects params to point to a boot_splash_config.
lv_obj_t *boot_splash_create(app_ctx &ctx, const void *params,
                              lv_group_t **group_out);

}  // namespace blusys::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
