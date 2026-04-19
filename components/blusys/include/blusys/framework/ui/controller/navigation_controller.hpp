#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/composition/screen_registry.hpp"
#include "blusys/framework/ui/widgets/overlay.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace blusys {

class screen_router;
struct shell;
struct screen_registration;
struct shell_tab_item;

// navigation_controller — single owner of screen routing, overlays,
// focus management, and shell chrome. Product code interacts only through
// this type (via fx.nav); screen_router, overlay_manager, and
// focus_scope_manager are private implementation details.
//
// Chrome updates (back button, tab highlight, title) happen in
// on_transition() — the one place they live.
class navigation_controller final {
public:
    void bind(screen_router *router, shell *s);

    // ---- screen registration ----

    bool register_screen(std::uint32_t          route_id,
                         screen_create_fn       create,
                         screen_destroy_fn      destroy   = nullptr,
                         const screen_lifecycle &lifecycle = {});

    template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
    bool register_screen(RouteEnum              route,
                         screen_create_fn       create,
                         screen_destroy_fn      destroy   = nullptr,
                         const screen_lifecycle &lifecycle = {})
    {
        return register_screen(static_cast<std::uint32_t>(route), create, destroy, lifecycle);
    }

    bool register_screens(app_ctx *ctx, const screen_registration *rows, std::size_t count);

    // ---- overlay management ----

    bool register_overlay(std::uint32_t id, lv_obj_t *overlay);

    template <typename OverlayEnum, typename = std::enable_if_t<std::is_enum_v<OverlayEnum>>>
    bool register_overlay(OverlayEnum id, lv_obj_t *overlay)
    {
        return register_overlay(static_cast<std::uint32_t>(id), overlay);
    }

    lv_obj_t *create_overlay(lv_obj_t *parent, std::uint32_t id, const overlay_config &config);

    template <typename OverlayEnum, typename = std::enable_if_t<std::is_enum_v<OverlayEnum>>>
    lv_obj_t *create_overlay(lv_obj_t *parent, OverlayEnum id, const overlay_config &config)
    {
        return create_overlay(parent, static_cast<std::uint32_t>(id), config);
    }

    // ---- tab items ----

    void set_tab_items(const shell_tab_item *items, std::size_t count);

    // ---- shell surface accessors ----

    [[nodiscard]] bool      has_shell()      const { return shell_ != nullptr; }
    void                    set_title(const char *title) const;
    [[nodiscard]] lv_obj_t *shell_root()     const;
    [[nodiscard]] lv_obj_t *status_surface()  const;
    [[nodiscard]] lv_obj_t *content_area()   const;

    // ---- route dispatch (used by nav_fx) ----

    void submit(const route_command &cmd);

private:
    static void transition_trampoline(lv_obj_t *screen, void *user_ctx);
    void on_transition();

    screen_router *router_ = nullptr;
    shell         *shell_  = nullptr;
};

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
