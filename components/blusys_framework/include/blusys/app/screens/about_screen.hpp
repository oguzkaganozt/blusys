#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/app_ctx.hpp"
#include "lvgl.h"

#include <cstddef>

namespace blusys::app::screens {

struct about_extra_field {
    const char *key   = nullptr;
    const char *value = nullptr;
};

struct about_screen_config {
    const char *product_name      = nullptr;
    const char *firmware_version  = nullptr;
    const char *hardware_version  = nullptr;
    const char *serial_number     = nullptr;
    const char *build_date        = nullptr;
    const about_extra_field *extras = nullptr;
    std::size_t extra_count        = 0;

    // When true, appends chip model, free heap, min heap, uptime, and IDF
    // version from `ctx.diagnostics()` when the diagnostics capability is composed.
    bool fill_diagnostics_from_ctx = false;
};

// screen_create_fn compatible — register with screen_router.
// Expects params to point to an about_screen_config.
lv_obj_t *about_screen_create(app_ctx &ctx, const void *params,
                               lv_group_t **group_out);

}  // namespace blusys::app::screens

#endif  // BLUSYS_FRAMEWORK_HAS_UI
