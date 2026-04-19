#pragma once

#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/capabilities/persistence.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/capabilities/build_info.hpp"
#include "blusys/framework/events/router.hpp"
#include "blusys/framework/observe/snapshot.h"

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

// Typed effect bridge for navigation, storage, diagnostics, and build info.
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

    class storage_fx {
    public:
        [[nodiscard]] blusys_fs_t *spiffs() const
        {
            return storage_ != nullptr ? storage_->spiffs() : nullptr;
        }

        [[nodiscard]] blusys_fatfs_t *fatfs() const
        {
            return storage_ != nullptr ? storage_->fatfs() : nullptr;
        }

    private:
        friend class app_fx;

        void bind(storage_capability *storage) noexcept { storage_ = storage; }

        storage_capability *storage_ = nullptr;
    };

    // Typed settings surface backed by persistence_capability.
    // Returns BLUSYS_ERR_INVALID_STATE if no persistence_capability is composed.
    class settings_fx {
    public:
        [[nodiscard]] bool is_ready() const
        {
            return persistence_ != nullptr && persistence_->status().nvs_ready;
        }

        blusys_err_t get_u32(const char* key, std::uint32_t* out) const
        {
            return persistence_ != nullptr
                ? persistence_->app_get_u32(key, out)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t set_u32(const char* key, std::uint32_t value)
        {
            return persistence_ != nullptr
                ? persistence_->app_set_u32(key, value)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t get_i32(const char* key, std::int32_t* out) const
        {
            return persistence_ != nullptr
                ? persistence_->app_get_i32(key, out)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t set_i32(const char* key, std::int32_t value)
        {
            return persistence_ != nullptr
                ? persistence_->app_set_i32(key, value)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t get_str(const char* key, char* out, std::size_t* len) const
        {
            return persistence_ != nullptr
                ? persistence_->app_get_str(key, out, len)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t set_str(const char* key, const char* value)
        {
            return persistence_ != nullptr
                ? persistence_->app_set_str(key, value)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t get_blob(const char* key, void* out, std::size_t* len) const
        {
            return persistence_ != nullptr
                ? persistence_->app_get_blob(key, out, len)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t set_blob(const char* key, const void* value, std::size_t len)
        {
            return persistence_ != nullptr
                ? persistence_->app_set_blob(key, value, len)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t erase(const char* key)
        {
            return persistence_ != nullptr
                ? persistence_->app_erase(key)
                : BLUSYS_ERR_INVALID_STATE;
        }

    private:
        friend class app_fx;
        void bind(persistence_capability* p) noexcept { persistence_ = p; }
        persistence_capability* persistence_ = nullptr;
    };

    class diagnostics_fx {
    public:
        [[nodiscard]] const diagnostics_status *status() const
        {
            return diagnostics_ != nullptr ? &diagnostics_->status() : nullptr;
        }

        [[nodiscard]] const diagnostics_snapshot *snapshot() const
        {
            const auto *s = status();
            return s != nullptr ? &s->last_snapshot : nullptr;
        }

        [[nodiscard]] const blusys_observe_snapshot_t *observability() const
        {
            const auto *s = status();
            return s != nullptr ? &s->observability : nullptr;
        }

        [[nodiscard]] const blusys_observe_snapshot_t *crash_dump() const
        {
            const auto *s = status();
            return s != nullptr && s->crash_dump_ready ? &s->crash_dump : nullptr;
        }

        [[nodiscard]] const blusys_observe_last_error_t *last_error(blusys_err_domain_t domain) const
        {
            const auto *s = status();
            if (s == nullptr || (unsigned)domain >= (unsigned)err_domain_count) {
                return nullptr;
            }
            const auto &e = s->observability.last_errors[domain];
            return e.valid ? &e : nullptr;
        }

        [[nodiscard]] std::uint32_t counter(blusys_err_domain_t domain) const
        {
            const auto *s = status();
            if (s == nullptr || (unsigned)domain >= (unsigned)err_domain_count) {
                return 0;
            }
            return s->observability.counters.values[domain];
        }

    private:
        friend class app_fx;

        void bind(diagnostics_capability *diag) noexcept { diagnostics_ = diag; }

        diagnostics_capability *diagnostics_ = nullptr;
    };

    class build_fx {
    public:
        [[nodiscard]] const build_info_status *status() const
        {
            return build_ != nullptr ? &build_->status() : nullptr;
        }

        [[nodiscard]] const char *firmware_version() const
        {
            const auto *s = status();
            return s != nullptr && s->firmware_version != nullptr ? s->firmware_version : "unknown";
        }

        [[nodiscard]] const char *build_host() const
        {
            const auto *s = status();
            return s != nullptr && s->build_host != nullptr ? s->build_host : "unknown";
        }

        [[nodiscard]] const char *commit_hash() const
        {
            const auto *s = status();
            return s != nullptr && s->commit_hash != nullptr ? s->commit_hash : "unknown";
        }

        [[nodiscard]] const char *feature_flags() const
        {
            const auto *s = status();
            return s != nullptr && s->feature_flags != nullptr ? s->feature_flags : "unknown";
        }

    private:
        friend class app_fx;

        void bind(build_info_capability *build) noexcept { build_ = build; }

        build_info_capability *build_ = nullptr;
    };

    nav_fx nav;
    storage_fx storage;
    settings_fx settings;
    diagnostics_fx diag;
    build_fx build;

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
