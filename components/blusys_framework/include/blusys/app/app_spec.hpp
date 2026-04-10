#pragma once

#include "blusys/app/app_ctx.hpp"
#include "blusys/framework/core/intent.hpp"

#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/theme.hpp"
#endif

namespace blusys::app {

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

    // ---- runtime tuning ----
    std::uint32_t tick_period_ms = 10;

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    // ---- theme (interactive path only) ----
    // nullptr = use default dark preset.
    const blusys::ui::theme_tokens *theme = nullptr;
#endif
};

}  // namespace blusys::app
