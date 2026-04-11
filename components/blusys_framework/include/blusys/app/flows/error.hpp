#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/app_ctx.hpp"
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

template<typename Action>
struct error_panel_dispatch_scratch {
    app_ctx *ctx = nullptr;
    Action     retry_action{};
    Action     dismiss_action{};
    bool       has_dismiss = false;
};

template<typename Action>
void error_panel_dispatch_retry_cb(void *user_data)
{
    auto *s = static_cast<error_panel_dispatch_scratch<Action> *>(user_data);
    if (s != nullptr && s->ctx != nullptr) {
        s->ctx->dispatch(s->retry_action);
    }
}

template<typename Action>
void error_panel_dispatch_dismiss_cb(void *user_data)
{
    auto *s = static_cast<error_panel_dispatch_scratch<Action> *>(user_data);
    if (s != nullptr && s->ctx != nullptr && s->has_dismiss) {
        s->ctx->dispatch(s->dismiss_action);
    }
}

// Same as `error_panel_create`, but routes buttons through `app_ctx::dispatch`.
// One active panel per `Action` type (static scratch).
template<typename Action>
lv_obj_t *error_panel_dispatch(lv_obj_t *parent, app_ctx &ctx,
                                const error_display_config &config,
                                const Action &retry_action,
                                const Action *dismiss_action = nullptr)
{
    static error_panel_dispatch_scratch<Action> scratch;
    scratch.ctx           = &ctx;
    scratch.retry_action  = retry_action;
    scratch.has_dismiss   = dismiss_action != nullptr;
    if (scratch.has_dismiss) {
        scratch.dismiss_action = *dismiss_action;
    }

    return error_panel_create(parent, config,
                              error_panel_dispatch_retry_cb<Action>,
                              &scratch,
                              scratch.has_dismiss
                                  ? error_panel_dispatch_dismiss_cb<Action>
                                  : nullptr,
                              scratch.has_dismiss ? &scratch : nullptr);
}

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
