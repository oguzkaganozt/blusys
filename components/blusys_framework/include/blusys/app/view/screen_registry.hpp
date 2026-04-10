#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/core/containers.hpp"
#include "blusys/framework/core/router.hpp"

#include <cstddef>
#include <cstdint>

#include "lvgl.h"

namespace blusys::app { class app_ctx; }

namespace blusys::app::view {

// Function pointer types for screen lifecycle.
//
// create_fn  — builds the screen LVGL objects; returns the screen lv_obj_t*
//              and optionally writes a focus group pointer to *group_out.
//              Called by the framework when navigating to a route.
// destroy_fn — tears down a screen before the next one is loaded.
//              nullptr = framework calls lv_obj_delete automatically.
using screen_create_fn  = lv_obj_t* (*)(app_ctx &ctx, const void *params,
                                         lv_group_t **group_out);
using screen_destroy_fn = void (*)(lv_obj_t *screen);

// Manages a fixed-capacity screen registry and a navigation stack.
//
// Products register screens by route ID during on_init. The framework
// then owns screen creation and teardown on every navigation command.
// No dynamic allocation — all storage is fixed-capacity.
class screen_registry {
public:
    static constexpr std::size_t kMaxScreens    = 8;
    static constexpr std::size_t kMaxStackDepth = 4;

    // Register a screen factory for a given route ID.
    // destroy_fn may be nullptr — the framework will call lv_obj_delete.
    // Returns false if the registry is full.
    bool register_screen(std::uint32_t    route_id,
                         screen_create_fn  create_fn,
                         screen_destroy_fn destroy_fn = nullptr);

    // Execute a navigation command. Returns true if the route was found
    // and the navigation completed. show_overlay / hide_overlay are not
    // handled here — the screen_router delegates those to overlay_manager.
    bool navigate(const blusys::framework::route_command &command, app_ctx &ctx);

    // Set a callback invoked after every screen load.
    // The device entry path uses this to call input_bridge_attach_screen.
    void set_screen_changed_callback(void (*fn)(lv_obj_t *screen, void *user_ctx),
                                     void *user_ctx);

private:
    struct screen_entry {
        std::uint32_t     route_id   = 0;
        screen_create_fn  create_fn  = nullptr;
        screen_destroy_fn destroy_fn = nullptr;
    };

    const screen_entry *find(std::uint32_t id) const;
    void load_screen(std::uint32_t route_id, const void *params, app_ctx &ctx);
    void destroy_active();

    blusys::static_vector<screen_entry, kMaxScreens>     entries_{};
    blusys::static_vector<std::uint32_t, kMaxStackDepth> nav_stack_{};

    lv_obj_t          *active_screen_     = nullptr;
    screen_destroy_fn  active_destroy_fn_ = nullptr;

    void (*on_screen_changed_)(lv_obj_t *screen, void *user_ctx) = nullptr;
    void *screen_changed_ctx_ = nullptr;
};

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
