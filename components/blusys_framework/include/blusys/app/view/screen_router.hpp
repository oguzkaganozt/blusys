#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/overlay_manager.hpp"
#include "blusys/app/view/screen_registry.hpp"
#include "blusys/framework/core/router.hpp"
#include "blusys/framework/ui/input/focus_scope.hpp"

#include <cstdint>

#include "lvgl.h"

namespace blusys::app { class app_ctx; }

namespace blusys::app::view {

// Combined route_sink that owns screen navigation and overlay management.
//
// Dispatches:
//   - show_overlay / hide_overlay  →  overlay_manager
//   - all other route commands     →  screen_registry
//
// This is the framework-owned route sink used for interactive products.
// Products compose their navigation in on_init:
//
//   ctx.screen_router()->register_screen(RouteId::home, &create_home);
//   ctx.navigate_to(RouteId::home);
//
//   ctx.overlay_manager()->register_overlay(OverlayId::modal, overlay);
//   ctx.show_overlay(OverlayId::modal);
class screen_router final : public blusys::framework::route_sink {
public:
    void submit(const blusys::framework::route_command &command) override;

    // Register a screen by route ID. Called during on_init.
    bool register_screen(std::uint32_t    route_id,
                         screen_create_fn  create_fn,
                         screen_destroy_fn destroy_fn = nullptr);

    // Register a screen with lifecycle hooks.
    bool register_screen(std::uint32_t         route_id,
                         screen_create_fn       create_fn,
                         screen_destroy_fn      destroy_fn,
                         const screen_lifecycle &lifecycle);

    // Access the overlay manager for overlay registration.
    [[nodiscard]] view::overlay_manager &overlays() { return overlays_; }

    // Set a callback invoked after each screen transition (for focus
    // re-attachment). The device entry path sets this to call
    // input_bridge_attach_screen after the input_bridge is created.
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
    [[nodiscard]] blusys::ui::focus_scope_manager &focus_scopes() { return focus_scopes_; }

    // Bind a shell content area. When bound, screens are loaded as
    // children of content_area instead of as standalone lv_screens.
    void bind_shell(lv_obj_t *content_area) { screens_.bind_shell(content_area); }

    // Current navigation stack depth.
    [[nodiscard]] std::size_t stack_depth() const { return screens_.stack_depth(); }

private:
    screen_registry                  screens_{};
    overlay_manager                  overlays_{};
    blusys::ui::focus_scope_manager  focus_scopes_{};
    app_ctx                         *ctx_ = nullptr;
};

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
