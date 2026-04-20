#pragma once

#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/framework/capabilities/persistence.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/capabilities/build_info.hpp"
#include "blusys/framework/events/router.hpp"
#include "blusys/observe/snapshot.h"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/controller/navigation_controller.hpp"
#endif

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace blusys {

class app_runtime_base;
class navigation_controller;
class diagnostics_capability;
class build_info_capability;

// Typed effect bridge for navigation plus capability-owned connectivity/
// storage/telemetry/settings/diagnostics/build namespaces.
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

        [[nodiscard]] bool has_shell() const
        {
            return ctrl_ != nullptr && ctrl_->has_shell();
        }

        void set_title(const char *title) const
        {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
            if (ctrl_ != nullptr) {
                ctrl_->set_title(title);
            }
#else
            (void)title;
#endif
        }

        [[nodiscard]] lv_obj_t *shell_root() const
        {
            return ctrl_ != nullptr ? ctrl_->shell_root() : nullptr;
        }

        [[nodiscard]] lv_obj_t *status_surface() const
        {
            return ctrl_ != nullptr ? ctrl_->status_surface() : nullptr;
        }

        void set_tab_items(const ::blusys::shell_tab_item *items, std::size_t count)
        {
            if (ctrl_ != nullptr) ctrl_->set_tab_items(items, count);
        }

        [[nodiscard]] lv_obj_t *content_area() const
        {
            return ctrl_ != nullptr ? ctrl_->content_area() : nullptr;
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

    using connectivity_fx = connectivity_capability::effects;
    using storage_fx      = storage_capability::effects;
    using telemetry_fx    = telemetry_capability::effects;
    using settings_fx     = persistence_capability::effects;
    using diagnostics_fx  = diagnostics_capability::effects;
    using build_fx        = build_info_capability::effects;

    nav_fx nav;
    connectivity_fx connectivity;
    storage_fx storage;
    telemetry_fx telemetry;
    settings_fx settings;
    diagnostics_fx diag;
    build_fx build;

private:
    friend class app_runtime_base;

    void bind_navigation(::blusys::navigation_controller *ctrl) noexcept
    {
        nav.bind(ctrl);
    }

    void bind_connectivity(connectivity_capability *conn_cap) noexcept
    {
        connectivity.bind(conn_cap);
    }

    void bind_storage(storage_capability *storage_cap) noexcept
    {
        storage.bind(storage_cap);
    }

    void bind_telemetry(telemetry_capability *telemetry_cap) noexcept
    {
        telemetry.bind(telemetry_cap);
    }

    void bind_persistence(persistence_capability *persistence_cap) noexcept
    {
        settings.bind(persistence_cap);
    }

    void bind_diagnostics(diagnostics_capability *diag_cap) noexcept
    {
        diag.bind(diag_cap);
    }

    void bind_build(build_info_capability *build_cap) noexcept
    {
        build.bind(build_cap);
    }
};

}  // namespace blusys
