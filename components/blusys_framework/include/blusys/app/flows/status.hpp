#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/app_ctx.hpp"
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

template<typename Action>
struct confirmation_dispatch_scratch {
    app_ctx *ctx = nullptr;
    Action     confirm_action{};
    Action     cancel_action{};
};

template<typename Action>
void confirmation_dispatch_confirm_cb(void *user_data)
{
    auto *s = static_cast<confirmation_dispatch_scratch<Action> *>(user_data);
    if (s != nullptr && s->ctx != nullptr) {
        s->ctx->dispatch(s->confirm_action);
    }
}

template<typename Action>
void confirmation_dispatch_cancel_cb(void *user_data)
{
    auto *s = static_cast<confirmation_dispatch_scratch<Action> *>(user_data);
    if (s != nullptr && s->ctx != nullptr) {
        s->ctx->dispatch(s->cancel_action);
    }
}

template<typename Action>
lv_obj_t *confirmation_dispatch(lv_obj_t *parent, app_ctx &ctx,
                                 const confirmation_config &config,
                                 const Action &confirm_action,
                                 const Action &cancel_action)
{
    static confirmation_dispatch_scratch<Action> scratch;
    scratch.ctx            = &ctx;
    scratch.confirm_action = confirm_action;
    scratch.cancel_action  = cancel_action;

    return confirmation_create(parent, config,
                               confirmation_dispatch_confirm_cb<Action>,
                               &scratch,
                               confirmation_dispatch_cancel_cb<Action>,
                               &scratch);
}

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
