#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/containers.hpp"
#include "blusys/framework/events/router.hpp"

#include <cstddef>
#include <cstdint>

#include "lvgl.h"

namespace blusys {
    class focus_scope_manager;
    enum class transition_type : std::uint32_t;
}
namespace blusys { class app_ctx; }

namespace blusys {

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

// Per-screen lifecycle hooks.
//
// Products can optionally provide these when registering a screen.
// The framework calls them at the appropriate navigation boundaries.
//
// Call order on push: old.on_focus_lost → old.on_hide → new.on_show → new.on_focus_gained
// Call order on pop:  top.on_focus_lost → top.on_hide → prev.on_show → prev.on_focus_gained
struct screen_lifecycle {
    void (*on_show)(lv_obj_t *screen, void *user_data)         = nullptr;
    void (*on_hide)(lv_obj_t *screen, void *user_data)         = nullptr;
    void (*on_focus_gained)(lv_obj_t *screen, void *user_data) = nullptr;
    void (*on_focus_lost)(lv_obj_t *screen, void *user_data)   = nullptr;
    void *user_data                                            = nullptr;
};

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
    bool register_screen(std::uint32_t          route_id,
                         screen_create_fn       create_fn,
                         screen_destroy_fn      destroy_fn = nullptr,
                         const screen_lifecycle &lifecycle = {});

    // Execute a navigation command. Returns true if the route was found
    // and the navigation completed. show_overlay / hide_overlay are not
    // handled here — the screen_router delegates those to overlay_manager.
    bool navigate(const blusys::route_command &command, app_ctx &ctx);

    // Register a callback invoked after every screen load.
    // Multiple callbacks may be registered; the device entry path uses
    // this to call input_bridge_attach_screen and the shell uses it to
    // keep its chrome in sync with the nav stack.
    void set_screen_changed_callback(void (*fn)(lv_obj_t *screen, void *user_ctx),
                                     void *user_ctx);

    // Bind a focus scope manager. When bound, the screen_registry resets
    // and pushes a new focus scope on each screen load instead of the
    // ad-hoc group-to-indev wiring.
    void bind_focus_scope(blusys::focus_scope_manager *fsm) { focus_scope_ = fsm; }

    // Bind a shell. When bound, screen content is created as a child of
    // the shell's content_area and the old content is deleted on
    // navigation. The shell's header auto-updates title and back button.
    void bind_shell(lv_obj_t *content_area) { shell_content_ = content_area; }

    // Current navigation stack depth (for shell header back-button logic).
    [[nodiscard]] std::size_t stack_depth() const { return nav_stack_.size(); }

    // Stack index 0 is the bottom (first pushed / root). Used by shell tab sync.
    [[nodiscard]] std::size_t nav_stack_size() const { return nav_stack_.size(); }
    [[nodiscard]] std::uint32_t nav_route_at(std::size_t index_from_bottom) const;

private:
    struct screen_entry {
        std::uint32_t     route_id   = 0;
        screen_create_fn  create_fn  = nullptr;
        screen_destroy_fn destroy_fn = nullptr;
        screen_lifecycle  lifecycle  = {};
    };

    // Navigation stack entry — preserves route ID and params for back-navigation.
    struct nav_entry {
        std::uint32_t  route_id = 0;
        const void    *params   = nullptr;
    };

    struct screen_changed_callback_entry {
        void (*fn)(lv_obj_t *screen, void *user_ctx) = nullptr;
        void *user_ctx = nullptr;
    };

    const screen_entry *find(std::uint32_t id) const;
    bool load_screen(std::uint32_t route_id, const void *params, app_ctx &ctx,
                     std::uint32_t transition_field, blusys::transition_type default_transition);
    void destroy_active();
    void fire_hide_hooks();
    void fire_show_hooks(lv_obj_t *screen, const screen_lifecycle &lifecycle);
    void finalize_load(lv_obj_t *screen, lv_group_t *group,
                       const screen_lifecycle &lifecycle);

    blusys::static_vector<screen_entry, kMaxScreens>  entries_{};
    blusys::static_vector<nav_entry, kMaxStackDepth>  nav_stack_{};

    lv_obj_t          *active_screen_     = nullptr;
    screen_destroy_fn  active_destroy_fn_ = nullptr;
    screen_lifecycle   active_lifecycle_   = {};

    blusys::static_vector<screen_changed_callback_entry, 4> screen_changed_callbacks_{};

    blusys::focus_scope_manager *focus_scope_    = nullptr;
    lv_obj_t                        *shell_content_  = nullptr;
};

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
