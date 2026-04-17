#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

// Settings screen builder — declarative `setting_item` rows plus a single
// `on_changed` bridge. Compose in `ui/` or register via a settings_flow in
// spec.flows (preferred — avoids direct screen registration in on_init).

#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/flows/flow_base.hpp"
#include "blusys/framework/ui/composition/screen_registry.hpp"
#include "lvgl.h"

#include <cstddef>
#include <cstdint>

namespace blusys::flows {

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

    // Stable id for on_changed (non-zero). If 0, the table index is passed instead.
    std::uint32_t id          = 0;

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
    // setting_id is item.id when non-zero, otherwise the row index in `items`.
    // value is: toggle (0/1), slider (new value), dropdown (index), button (0).
    void (*on_changed)(void *ctx, std::uint32_t setting_id, std::int32_t value) = nullptr;
    void *user_ctx = nullptr;
};

// screen_create_fn compatible — register with screen_router.
// Expects params to point to a settings_screen_config.
lv_obj_t *settings_screen_create(app_ctx &ctx, const void *params,
                                  lv_group_t **group_out);

// settings_flow — registers a settings screen via spec.flows.
//
// The create_fn receives app_ctx at render time and may read product state
// to populate initial widget values. Lifecycle callbacks (on_show, on_hide)
// receive the ctx pointer passed to register_screens as user_data.
//
// Usage:
//   static blusys::flows::settings_flow kSettings = []() {
//       blusys::flows::settings_flow f;
//       f.route_id  = static_cast<uint32_t>(route::settings);
//       f.create_fn = &my_create_settings;
//       return f;
//   }();
//   // In app_main.cpp spec: .flows = {&kSettings}, .flow_count = 1
class settings_flow : public flow_base {
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
