#pragma once

#include "blusys/app/app_ctx.hpp"
#include "blusys/app/app_identity.hpp"
#include "blusys/framework/core/intent.hpp"

#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/theme.hpp"
#endif

namespace blusys::app::view { struct shell_config; }

namespace blusys::app {

struct capability_list;  // forward declaration — include capability_list.hpp to use

// Application definition consumed by app_runtime. Use designated initializers in this order:
//
//   1. initial_state     (required)
//   2. update            (required)
//   3. on_init            — screen registration, first navigation
//   4. on_tick            — periodic work; keep light
//   5. map_intent         — interactive: framework intents → Action
//   6. map_event          — capabilities → Action (see blusys/app/integration.hpp)
//   7. tick_period_ms
//   8. capabilities       — device only; nullptr on host
//   9. identity           — theme + feedback preset
//  10. theme / shell      — interactive only (when BLUSYS_FRAMEWORK_HAS_UI)
//
// Optional `map_intent` / `map_event` may be nullptr; those inputs are then ignored.
//
// Alternative action models: you may use `std::variant<...>` as `Action` if every
// alternative is dispatchable from `ctx.dispatch`. Use `blusys/app/integration.hpp`
// if you need `dispatch_variant` from outside the reducer.

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
    // identity->theme takes precedence over spec.theme.
    const app_identity *identity = nullptr;

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    // ---- theme (interactive path only) ----
    // nullptr = use default expressive_dark preset.
    // Prefer setting identity->theme instead of this field.
    const blusys::ui::theme_tokens *theme = nullptr;

    // ---- interaction shell (interactive path only) ----
    // When non-null, the framework creates a persistent shell with
    // header, status bar, and/or tab bar. Screen content is swapped
    // inside the shell's content area on navigation. nullptr = no
    // shell, screens are standalone (current default behavior).
    const view::shell_config *shell = nullptr;
#endif
};

}  // namespace blusys::app
