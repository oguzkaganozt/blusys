#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

// Settings screen builder — declarative `setting_item` rows plus a single
// `on_changed` bridge. Compose in `ui/` or register as a router screen via
// `settings_screen_config` params.

#include "blusys/app/app_ctx.hpp"
#include "lvgl.h"

#include <cstddef>
#include <cstdint>

namespace blusys::app::flows {

enum class setting_type : std::uint8_t {
    toggle,
    slider,
    dropdown,
    action_button,
    info,
    section_header,
};

struct setting_item {
    const char   *label       = nullptr;
    const char   *description = nullptr;
    setting_type  type        = setting_type::info;

    // toggle
    bool toggle_initial = false;

    // slider
    std::int32_t slider_min     = 0;
    std::int32_t slider_max     = 100;
    std::int32_t slider_initial = 0;

    // dropdown
    const char * const *dropdown_options = nullptr;
    std::int32_t dropdown_count          = 0;
    std::int32_t dropdown_initial        = 0;

    // info (read-only key-value)
    const char *info_value = nullptr;

    // action_button
    const char *button_label = nullptr;
};

struct settings_screen_config {
    const char         *title      = "Settings";
    const setting_item *items      = nullptr;
    std::size_t         item_count = 0;
    // Called when a setting value changes.
    // item_index identifies which setting_item changed.
    // value is: toggle (0/1), slider (new value), dropdown (index), button (0).
    void (*on_changed)(void *ctx, std::size_t item_index, std::int32_t value) = nullptr;
    void *user_ctx = nullptr;
};

// screen_create_fn compatible — register with screen_router.
// Expects params to point to a settings_screen_config.
lv_obj_t *settings_screen_create(app_ctx &ctx, const void *params,
                                  lv_group_t **group_out);

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
