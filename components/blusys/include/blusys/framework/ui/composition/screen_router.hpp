#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/composition/overlay_manager.hpp"
#include "blusys/framework/ui/composition/screen_registry.hpp"
#include "blusys/framework/engine/router.hpp"
#include "blusys/framework/ui/input/focus_scope.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "lvgl.h"

namespace blusys { class app_ctx; }

namespace blusys {

struct shell;

// Bulk registration row — lifecycle user_data is set to ctx from register_screens().
struct screen_registration {
    std::uint32_t    route_id   = 0;
    screen_create_fn create     = nullptr;
    screen_destroy_fn destroy   = nullptr;
    void (*on_show)(lv_obj_t *, void *) = nullptr;
    void (*on_hide)(lv_obj_t *, void *) = nullptr;
};

// Build a registration row from an enum class route id (avoids static_cast in tables).
template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
[[nodiscard]] constexpr screen_registration screen_row(RouteEnum route,
                                                       screen_create_fn create,
                                                       screen_destroy_fn destroy   = nullptr,
                                                       void (*on_show)(lv_obj_t *, void *) = nullptr,
                                                       void (*on_hide)(lv_obj_t *, void *) = nullptr)
{
    return screen_registration{
        static_cast<std::uint32_t>(route),
        create,
        destroy,
        on_show,
        on_hide,
    };
}

// Combined route_sink that owns screen navigation and overlay management.
//
// Dispatches:
//   - show_overlay / hide_overlay  →  overlay_manager
//   - all other route commands     →  screen_registry
//
// This is the framework-owned route sink used for interactive products.
// Products compose their navigation in on_init:
//
//   ctx.services().screen_router()->register_screen(RouteId::home, &create_home);
//   ctx.services().navigate_to(RouteId::home);
//
//   ctx.services().overlay_manager()->register_overlay(OverlayId::modal, overlay);
//   ctx.services().show_overlay(OverlayId::modal);
//
// Tab routes (shell tab bar) use navigate_to(route_id) — one stack slot per tab.
// Pushed flows (modals, wizards) use navigate_push / navigate_back; shell header
// back button reflects stack depth (see shell.hpp).
class screen_router final : public blusys::route_sink {
public:
    void submit(const blusys::route_command &command) override;

    // Register a screen by route ID. Called during on_init.
    bool register_screen(std::uint32_t          route_id,
                         screen_create_fn       create_fn,
                         screen_destroy_fn      destroy_fn = nullptr,
                         const screen_lifecycle &lifecycle = {});

    template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
    bool register_screen(RouteEnum              route_id,
                         screen_create_fn       create_fn,
                         screen_destroy_fn      destroy_fn = nullptr,
                         const screen_lifecycle &lifecycle = {})
    {
        return register_screen(static_cast<std::uint32_t>(route_id), create_fn, destroy_fn, lifecycle);
    }

    // Register many screens from a static table. Lifecycle user_data is ctx.
    bool register_screens(app_ctx *ctx, const screen_registration *rows, std::size_t count);

    // Access the overlay manager for overlay registration.
    [[nodiscard]] overlay_manager &overlays() { return overlays_; }

    // Set a callback invoked after each screen transition (focus refresh,
    // optional product hooks). The device entry path may replace this to
    // also call focus_scope_manager::refresh_current when using a hardware encoder.
    void set_screen_changed_callback(void (*fn)(lv_obj_t *screen, void *user_ctx),
                                     void *user_ctx);

    // Provide the app_ctx pointer used by screen create_fn calls.
    // Called by app_runtime during init, before on_init runs.
    void bind_ctx(app_ctx *ctx) {
        ctx_ = ctx;
        overlays_.bind_focus_scope(&focus_scopes_);
        screens_.bind_focus_scope(&focus_scopes_);
    }

    // Access the focus scope manager.
    [[nodiscard]] blusys::focus_scope_manager &focus_scopes() { return focus_scopes_; }

    // Bind a shell content area. When bound, screens are loaded as
    // children of content_area instead of as standalone lv_screens.
    void bind_shell(lv_obj_t *content_area) { screens_.bind_shell(content_area); }

    // Current navigation stack depth.
    [[nodiscard]] std::size_t stack_depth() const { return screens_.stack_depth(); }

    // Update shell tab bar + title from the navigation stack (call after each transition).
    void sync_shell_chrome(shell &s);

private:
    screen_registry                  screens_{};
    overlay_manager                  overlays_{};
    blusys::focus_scope_manager  focus_scopes_{};
    app_ctx                         *ctx_ = nullptr;
};

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
