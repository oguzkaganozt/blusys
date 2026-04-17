#pragma once

#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/engine/router.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/controller/navigation_controller.hpp"
#endif

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace blusys {

class app_runtime_base;
class navigation_controller;

#ifndef ESP_PLATFORM
struct blusys_fs;
typedef struct blusys_fs blusys_fs_t;
struct blusys_fatfs;
typedef struct blusys_fatfs blusys_fatfs_t;
#endif

// Typed effect bridge for navigation and storage.
//
// nav provides the full navigation and registration surface; product code
// never needs to reach the underlying screen_router, overlay_manager,
// focus_scope_manager, or shell directly.
class app_fx {
public:
    class nav_fx {
    public:
        void to(std::uint32_t route_id, const void *params = nullptr)
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            if (ctrl_ != nullptr) {
                ctrl_->submit(blusys::route::set_root(route_id, params));
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
            if (ctrl_ != nullptr) {
                ctrl_->submit(blusys::route::push(route_id, params));
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
            if (ctrl_ != nullptr) {
                ctrl_->submit(blusys::route::replace(route_id, params));
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
            if (ctrl_ != nullptr) {
                ctrl_->submit(blusys::route::pop());
            }
#endif
        }

        void show_overlay(std::uint32_t overlay_id, const void *params = nullptr)
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            if (ctrl_ != nullptr) {
                ctrl_->submit(blusys::route::show_overlay(overlay_id, params));
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
            if (ctrl_ != nullptr) {
                ctrl_->submit(blusys::route::hide_overlay(overlay_id));
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
            return ctrl_ != nullptr ? ctrl_->stack_depth() : 0;
#else
            return 0;
#endif
        }

        [[nodiscard]] bool can_navigate_back() const
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            return ctrl_ != nullptr && ctrl_->stack_depth() > 1;
#else
            return false;
#endif
        }

        // ---- registration API ----

#ifdef BLUSYS_FRAMEWORK_HAS_UI
        bool register_screen(std::uint32_t              route_id,
                             ::blusys::screen_create_fn  create,
                             ::blusys::screen_destroy_fn destroy   = nullptr,
                             const ::blusys::screen_lifecycle &lifecycle = {})
        {
            return ctrl_ != nullptr && ctrl_->register_screen(route_id, create, destroy, lifecycle);
        }

        template <typename RouteEnum, typename = std::enable_if_t<std::is_enum_v<RouteEnum>>>
        bool register_screen(RouteEnum                   route,
                             ::blusys::screen_create_fn  create,
                             ::blusys::screen_destroy_fn destroy   = nullptr,
                             const ::blusys::screen_lifecycle &lifecycle = {})
        {
            return register_screen(static_cast<std::uint32_t>(route), create, destroy, lifecycle);
        }

        bool register_screens(::blusys::app_ctx                    *ctx,
                              const ::blusys::screen_registration  *rows,
                              std::size_t                           count)
        {
            return ctrl_ != nullptr && ctrl_->register_screens(ctx, rows, count);
        }

        bool register_overlay(std::uint32_t id, lv_obj_t *overlay)
        {
            return ctrl_ != nullptr && ctrl_->register_overlay(id, overlay);
        }

        template <typename OverlayEnum, typename = std::enable_if_t<std::is_enum_v<OverlayEnum>>>
        bool register_overlay(OverlayEnum id, lv_obj_t *overlay)
        {
            return register_overlay(static_cast<std::uint32_t>(id), overlay);
        }

        lv_obj_t *create_overlay(lv_obj_t *parent, std::uint32_t id,
                                  const ::blusys::overlay_config &config)
        {
            return ctrl_ != nullptr ? ctrl_->create_overlay(parent, id, config) : nullptr;
        }

        template <typename OverlayEnum, typename = std::enable_if_t<std::is_enum_v<OverlayEnum>>>
        lv_obj_t *create_overlay(lv_obj_t *parent, OverlayEnum id,
                                  const ::blusys::overlay_config &config)
        {
            return create_overlay(parent, static_cast<std::uint32_t>(id), config);
        }

        void set_tab_items(const ::blusys::shell_tab_item *items, std::size_t count)
        {
            if (ctrl_ != nullptr) ctrl_->set_tab_items(items, count);
        }

        // ---- shell surface accessors ----

        [[nodiscard]] bool has_shell() const
        {
            return ctrl_ != nullptr && ctrl_->has_shell();
        }

        [[nodiscard]] lv_obj_t *content_area() const
        {
            return ctrl_ != nullptr ? ctrl_->content_area() : nullptr;
        }

        [[nodiscard]] lv_obj_t *shell_root() const
        {
            return ctrl_ != nullptr ? ctrl_->shell_root() : nullptr;
        }

        [[nodiscard]] lv_obj_t *status_surface() const
        {
            return ctrl_ != nullptr ? ctrl_->status_surface() : nullptr;
        }

        void set_title(const char *title)
        {
            if (ctrl_ != nullptr) ctrl_->set_title(title);
        }
#endif  // BLUSYS_FRAMEWORK_HAS_UI

    private:
        friend class app_fx;

        void bind(::blusys::navigation_controller *ctrl) noexcept
        {
            ctrl_ = ctrl;
        }

        ::blusys::navigation_controller *ctrl_ = nullptr;
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

    void bind_navigation(::blusys::navigation_controller *ctrl) noexcept
    {
        nav.bind(ctrl);
    }

    void bind_storage(storage_capability *storage_cap) noexcept
    {
        storage.bind(storage_cap);
    }
};

}  // namespace blusys
