#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/callbacks.hpp"
#include "lvgl.h"

namespace blusys::app::flows {

struct error_display_config {
    const char *title         = "Error";
    const char *message       = nullptr;
    const char *detail        = nullptr;
    const char *retry_label   = "Retry";
    const char *dismiss_label = nullptr;  // nullptr = no dismiss button
};

// Create an error panel with optional retry and dismiss buttons.
// on_retry fires when the user taps "Retry".
// on_dismiss fires when the user taps "Dismiss" (if dismiss_label != nullptr).
lv_obj_t *error_panel_create(lv_obj_t *parent,
                              const error_display_config &config,
                              blusys::ui::press_cb_t on_retry   = nullptr,
                              void *retry_ctx                    = nullptr,
                              blusys::ui::press_cb_t on_dismiss  = nullptr,
                              void *dismiss_ctx                  = nullptr);

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
