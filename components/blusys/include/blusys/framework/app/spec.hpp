#pragma once

#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/app/identity.hpp"
#include "blusys/framework/engine/intent.hpp"

#include <optional>
#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/style/theme.hpp"
#endif

namespace blusys { struct shell_config; }

namespace blusys {

struct capability_list;  // forward declaration — include capability_list.hpp to use

// Application definition consumed by app_runtime. Use designated initializers in this order:
//
//   1. initial_state     (required)
//   2. update            (required)
//   3. on_init            — screen registration, first navigation
//   4. on_tick            — periodic work; keep light
//   5. on_event           — unified event hook (phase 4)
//   6. tick_period_ms
//   7. capabilities       — device only; nullptr on host
//   8. identity           — theme + feedback preset
//   9. theme / shell      — interactive only (when BLUSYS_FRAMEWORK_HAS_UI)
//
// Optional `on_event` is the phase 4 path. It receives the framework event stream
// and may return an Action to dispatch.
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
    void (*on_init)(app_ctx &ctx, app_fx &fx, State &state) = nullptr;
    void (*on_tick)(app_ctx &ctx, app_fx &fx, State &state, std::uint32_t now_ms) = nullptr;

    // ---- unified event hook (phase 4) ----
    std::optional<Action> (*on_event)(blusys::event event, State &state) = nullptr;

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
    const blusys::theme_tokens *theme = nullptr;

    // ---- interaction shell (interactive path only) ----
    // When non-null, the framework creates a persistent shell with
    // header, status bar, and/or tab bar. Screen content is swapped
    // inside the shell's content area on navigation. nullptr = no
    // shell, screens are standalone (current default behavior).
    const shell_config *shell = nullptr;
#endif
};

}  // namespace blusys
