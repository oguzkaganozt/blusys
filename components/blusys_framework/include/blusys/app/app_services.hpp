#pragma once

// Framework-owned routing, UI chrome, and storage filesystem handles.
// Navigation and overlays are accessed from reducers via `app_ctx::services()`.

#include "blusys/framework/core/router.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
namespace blusys::app::view {
class overlay_manager;
class screen_router;
struct shell;
}
#endif

#ifdef ESP_PLATFORM
struct blusys_fs;
typedef struct blusys_fs blusys_fs_t;
struct blusys_fatfs;
typedef struct blusys_fatfs blusys_fatfs_t;
#endif

namespace blusys::app {

class app_runtime_base;
class storage_capability;  // ESP FS handles (via capability)

class app_services {
public:
    void navigate_to(std::uint32_t route_id, const void *params = nullptr);
    void navigate_push(std::uint32_t route_id, const void *params = nullptr);
    void navigate_replace(std::uint32_t route_id, const void *params = nullptr);
    void navigate_back();
    void show_overlay(std::uint32_t overlay_id, const void *params = nullptr);
    void hide_overlay(std::uint32_t overlay_id);

    template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
    void navigate_to(RouteEnum route, const void *params = nullptr)
    {
        navigate_to(static_cast<std::uint32_t>(route), params);
    }

    template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
    void navigate_push(RouteEnum route, const void *params = nullptr)
    {
        navigate_push(static_cast<std::uint32_t>(route), params);
    }

    template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
    void navigate_replace(RouteEnum route, const void *params = nullptr)
    {
        navigate_replace(static_cast<std::uint32_t>(route), params);
    }

    template <typename OverlayEnum, typename = std::enable_if_t<std::is_enum_v<OverlayEnum>>>
    void show_overlay(OverlayEnum overlay, const void *params = nullptr)
    {
        show_overlay(static_cast<std::uint32_t>(overlay), params);
    }

    template <typename OverlayEnum, typename = std::enable_if_t<std::is_enum_v<OverlayEnum>>>
    void hide_overlay(OverlayEnum overlay)
    {
        hide_overlay(static_cast<std::uint32_t>(overlay));
    }

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    [[nodiscard]] std::size_t navigation_stack_depth() const;
    [[nodiscard]] bool        can_navigate_back() const;

    [[nodiscard]] view::screen_router *screen_router() const { return screen_router_; }

    [[nodiscard]] view::overlay_manager *overlay_manager() const;

    [[nodiscard]] view::shell *shell() const { return shell_; }
#endif

#ifdef ESP_PLATFORM
    [[nodiscard]] blusys_fs_t    *spiffs() const;
    [[nodiscard]] blusys_fatfs_t *fatfs() const;
#endif

private:
    friend class app_runtime_base;

    void set_storage_for_fs(storage_capability *s) { storage_for_fs_ = s; }

    blusys::framework::route_sink *route_sink_ = nullptr;
#ifdef BLUSYS_FRAMEWORK_HAS_UI
    view::screen_router *screen_router_ = nullptr;
    view::shell         *shell_        = nullptr;
#endif
    storage_capability *storage_for_fs_ = nullptr;
};

}  // namespace blusys::app
