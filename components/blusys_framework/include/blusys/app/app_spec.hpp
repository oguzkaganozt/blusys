#pragma once

#include "blusys/app/app_ctx.hpp"
#include "blusys/app/app_identity.hpp"
#include "blusys/framework/core/intent.hpp"

#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/theme.hpp"
#endif

namespace blusys::app {

struct capability_list;  // forward declaration — include capability_list.hpp to use

template <typename State, typename Action>
struct app_spec {
    // ---- required ----
    State initial_state;
    void (*update)(app_ctx &ctx, State &state, const Action &action);

    // ---- optional lifecycle hooks ----
    void (*on_init)(app_ctx &ctx, State &state)                       = nullptr;
    void (*on_tick)(app_ctx &ctx, State &state, std::uint32_t now_ms) = nullptr;

    // ---- intent-to-action bridge (interactive path) ----
    // Return true and fill *out to map a framework intent to an app action.
    // If nullptr, framework intents are silently ignored (headless default).
    bool (*map_intent)(blusys::framework::intent intent, Action *out) = nullptr;

    // ---- integration event bridge (capabilities → app actions) ----
    // Called when a capability posts an integration event.
    // Return true and fill *out to convert it to an app action.
    // If nullptr, integration events are silently ignored.
    bool (*map_event)(std::uint32_t event_id, std::uint32_t event_code,
                      const void *payload, Action *out) = nullptr;

    // ---- runtime tuning ----
    std::uint32_t tick_period_ms = 10;

    // ---- capabilities (device path — ignored on host) ----
    // Pointer to a capability_list. The framework starts, polls, and stops
    // registered capabilities. nullptr = no capabilities.
    // Must outlive the app runtime.
    capability_list *capabilities = nullptr;

    // ---- product identity (optional) ----
    // When set, the runtime applies theme and feedback preset at boot.
    // identity->theme takes precedence over the legacy .theme field.
    const app_identity *identity = nullptr;

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    // ---- theme (interactive path only) ----
    // nullptr = use default expressive_dark preset.
    // Prefer setting identity->theme instead of this field.
    const blusys::ui::theme_tokens *theme = nullptr;
#endif
};

}  // namespace blusys::app
