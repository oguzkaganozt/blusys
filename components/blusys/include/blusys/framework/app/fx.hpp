#pragma once

#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/engine/router.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/composition/screen_router.hpp"
#endif

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace blusys {

class app_runtime_base;
class screen_router;
class overlay_manager;
struct shell;

#ifndef ESP_PLATFORM
struct blusys_fs;
typedef struct blusys_fs blusys_fs_t;
struct blusys_fatfs;
typedef struct blusys_fatfs blusys_fatfs_t;
#endif

// Typed effect bridge for navigation and storage.
//
// The surface is still narrow, but it is bound directly to runtime-owned
// router / shell / storage pointers instead of a mutable runtime bridge.
class app_fx {
public:
    class nav_fx {
    public:
        void to(std::uint32_t route_id, const void *params = nullptr)
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            if (router_ != nullptr) {
                router_->submit(blusys::route::set_root(route_id, params));
            }
#else
            (void)route_id;
            (void)params;
#endif
        }

        template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
        void to(RouteEnum route, const void *params = nullptr)
        {
            to(static_cast<std::uint32_t>(route), params);
        }

        void push(std::uint32_t route_id, const void *params = nullptr)
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            if (router_ != nullptr) {
                router_->submit(blusys::route::push(route_id, params));
            }
#else
            (void)route_id;
            (void)params;
#endif
        }

        template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
        void push(RouteEnum route, const void *params = nullptr)
        {
            push(static_cast<std::uint32_t>(route), params);
        }

        void replace(std::uint32_t route_id, const void *params = nullptr)
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            if (router_ != nullptr) {
                router_->submit(blusys::route::replace(route_id, params));
            }
#else
            (void)route_id;
            (void)params;
#endif
        }

        template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
        void replace(RouteEnum route, const void *params = nullptr)
        {
            replace(static_cast<std::uint32_t>(route), params);
        }

        void back()
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            if (router_ != nullptr) {
                router_->submit(blusys::route::pop());
            }
#endif
        }

        void show_overlay(std::uint32_t overlay_id, const void *params = nullptr)
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            if (router_ != nullptr) {
                router_->submit(blusys::route::show_overlay(overlay_id, params));
            }
#else
            (void)overlay_id;
            (void)params;
#endif
        }

        template <typename OverlayEnum, typename = std::enable_if_t<std::is_enum_v<OverlayEnum>>>
        void show_overlay(OverlayEnum overlay, const void *params = nullptr)
        {
            show_overlay(static_cast<std::uint32_t>(overlay), params);
        }

        void hide_overlay(std::uint32_t overlay_id)
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            if (router_ != nullptr) {
                router_->submit(blusys::route::hide_overlay(overlay_id));
            }
#else
            (void)overlay_id;
#endif
        }

        template <typename OverlayEnum, typename = std::enable_if_t<std::is_enum_v<OverlayEnum>>>
        void hide_overlay(OverlayEnum overlay)
        {
            hide_overlay(static_cast<std::uint32_t>(overlay));
        }

        [[nodiscard]] std::size_t navigation_stack_depth() const
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            return router_ != nullptr ? router_->stack_depth() : 0;
#else
            return 0;
#endif
        }

        [[nodiscard]] bool can_navigate_back() const
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            return router_ != nullptr && router_->stack_depth() > 1;
#else
            return false;
#endif
        }

        [[nodiscard]] ::blusys::screen_router *screen_router() const
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            return router_;
#else
            return nullptr;
#endif
        }

        [[nodiscard]] ::blusys::overlay_manager *overlay_manager() const
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            return router_ != nullptr ? &router_->overlays() : nullptr;
#else
            return nullptr;
#endif
        }

        [[nodiscard]] ::blusys::shell *shell() const
        {
            return shell_;
        }

    private:
        friend class app_fx;

        void bind(::blusys::screen_router *router, ::blusys::shell *shell) noexcept
        {
            router_ = router;
            shell_  = shell;
        }

        ::blusys::screen_router *router_ = nullptr;
        ::blusys::shell         *shell_  = nullptr;
    };

    class storage_fx {
    public:
#ifdef ESP_PLATFORM
        [[nodiscard]] blusys_fs_t *spiffs() const
        {
            return storage_ != nullptr ? storage_->spiffs() : nullptr;
        }

        [[nodiscard]] blusys_fatfs_t *fatfs() const
        {
            return storage_ != nullptr ? storage_->fatfs() : nullptr;
        }
#else
        [[nodiscard]] blusys_fs_t *spiffs() const { return nullptr; }
        [[nodiscard]] blusys_fatfs_t *fatfs() const { return nullptr; }
#endif

    private:
        friend class app_fx;

        void bind(storage_capability *storage) noexcept { storage_ = storage; }

        storage_capability *storage_ = nullptr;
    };

    nav_fx nav;
    storage_fx storage;

private:
    friend class app_runtime_base;

    void bind_navigation(::blusys::screen_router *router, ::blusys::shell *shell) noexcept
    {
        nav.bind(router, shell);
    }

    void bind_storage(storage_capability *storage_cap) noexcept
    {
        storage.bind(storage_cap);
    }
};

}  // namespace blusys
