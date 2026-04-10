#pragma once

#include "blusys/app/app_ctx.hpp"
#include "blusys/framework/core/intent.hpp"

#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/theme.hpp"
#endif

namespace blusys::app {

struct bundle_list;  // forward declaration — include bundle_list.hpp to use

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

    // ---- integration event bridge (service bundles → app actions) ----
    // Called when a bundle posts an integration event.
    // Return true and fill *out to convert it to an app action.
    // If nullptr, integration events are silently ignored.
    bool (*map_event)(std::uint32_t event_id, std::uint32_t event_code,
                      const void *payload, Action *out) = nullptr;

    // ---- runtime tuning ----
    std::uint32_t tick_period_ms = 10;

    // ---- bundles (device path — ignored on host) ----
    // Pointer to a bundle_list. The framework starts, polls, and stops
    // registered bundles. nullptr = no bundles.
    // Must outlive the app runtime.
    bundle_list *bundles = nullptr;

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    // ---- theme (interactive path only) ----
    // nullptr = use default dark preset.
    const blusys::ui::theme_tokens *theme = nullptr;
#endif
};

}  // namespace blusys::app
