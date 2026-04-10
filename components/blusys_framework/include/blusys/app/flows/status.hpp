#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/callbacks.hpp"
#include "blusys/framework/ui/primitives/status_badge.hpp"
#include "lvgl.h"

namespace blusys::app::flows {

struct alert_config {
    const char          *title   = nullptr;
    const char          *message = nullptr;
    blusys::ui::badge_level level = blusys::ui::badge_level::info;
    std::uint32_t auto_dismiss_ms = 0;  // 0 = manual dismiss only
};

// Create a toast/alert overlay panel.
lv_obj_t *alert_create(lv_obj_t *parent, const alert_config &config);

struct confirmation_config {
    const char *title         = "Confirm";
    const char *message       = nullptr;
    const char *confirm_label = "OK";
    const char *cancel_label  = "Cancel";
};

// Create a confirmation dialog with confirm/cancel buttons.
lv_obj_t *confirmation_create(lv_obj_t *parent,
                                const confirmation_config &config,
                                blusys::ui::press_cb_t on_confirm = nullptr,
                                void *confirm_ctx                  = nullptr,
                                blusys::ui::press_cb_t on_cancel   = nullptr,
                                void *cancel_ctx                   = nullptr);

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
