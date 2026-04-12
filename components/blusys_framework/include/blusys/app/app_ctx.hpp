#pragma once

#include "blusys/framework/core/feedback.hpp"
#include "blusys/framework/core/router.hpp"

#include "blusys/error.h"

#include <cassert>
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
// Forward declarations for C service handle types used by capability accessors.
struct blusys_fs;
typedef struct blusys_fs blusys_fs_t;
struct blusys_fatfs;
typedef struct blusys_fatfs blusys_fatfs_t;
#endif

namespace blusys::app {

class app_runtime_base;
struct capability_list;

// Forward declarations for capability status types.
struct connectivity_status;
struct storage_status;
struct bluetooth_status;
struct ota_status;
struct diagnostics_status;
struct telemetry_status;
struct provisioning_status;
class connectivity_capability;
class storage_capability;
class bluetooth_capability;
class ota_capability;
class diagnostics_capability;
class telemetry_capability;
class provisioning_capability;

class app_ctx {
public:
    // ---- navigation (wraps framework route helpers) ----

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
    // Current screen stack depth (1 = root). Zero when no router is bound.
    [[nodiscard]] std::size_t navigation_stack_depth() const;

    // True when navigate_back() would pop a pushed screen (same heuristic as shell back affordance).
    [[nodiscard]] bool can_navigate_back() const;
#endif

    // ---- feedback ----

    void emit_feedback(blusys::framework::feedback_channel channel,
                       blusys::framework::feedback_pattern pattern,
                       std::uint32_t value = 0);

    // ---- action dispatch (type-erased) ----
    //
    // Queues an action for the next reducer pass. Returns false if the action queue is full
    // (action dropped; runtime logs a warning) or if dispatch is not wired.

    template <typename Action>
    bool dispatch(const Action &action)
    {
        if (dispatch_fn_ != nullptr) {
            return dispatch_fn_(runtime_ptr_, static_cast<const void *>(&action));
        }
        return false;
    }

    // Pointer to the app-owned state (set by app_runtime for on_init / screen factories).
    // State must be the same type as app_spec<State, Action>::State. Casting to the wrong
    // type is undefined behavior. Not available after runtime deinit (nullptr).
    template <typename State>
    [[nodiscard]] State *product_state() const
    {
        assert(product_state_ != nullptr);
        return static_cast<State *>(product_state_);
    }

    [[nodiscard]] void *product_state_raw() const { return product_state_; }

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    // ---- screen router access (for registering screens in on_init) ----
    [[nodiscard]] view::screen_router *screen_router() const { return screen_router_; }

    // ---- overlay manager access (delegates to the screen_router's overlay_manager) ----
    [[nodiscard]] view::overlay_manager *overlay_manager() const;

    // ---- interaction shell access (nullptr if no shell configured) ----
    [[nodiscard]] view::shell *shell() const { return shell_; }
#endif

    // ---- capability status queries ----

    [[nodiscard]] const connectivity_status *connectivity() const;
    [[nodiscard]] const storage_status *storage() const;
    [[nodiscard]] const bluetooth_status *bluetooth() const;
    [[nodiscard]] const ota_status *ota() const;
    [[nodiscard]] const diagnostics_status *diagnostics() const;
    [[nodiscard]] const telemetry_status *telemetry() const;
    [[nodiscard]] const provisioning_status *provisioning() const;

#ifdef ESP_PLATFORM
    // Direct storage handle access — nullptr if the storage capability is absent
    // or the filesystem was not configured / failed to mount.
    [[nodiscard]] blusys_fs_t    *spiffs() const;
    [[nodiscard]] blusys_fatfs_t *fatfs()  const;
#endif

    // ---- capability requests (integration calls from intent bridges) ----
    // These forward to the composed capability when present; otherwise return
    // BLUSYS_ERR_INVALID_STATE.
    blusys_err_t request_connectivity_reconnect();

private:
    friend class app_runtime_base;

    static void bind_capability_pointers_from_list(app_ctx &ctx, capability_list *capabilities);

    using dispatch_fn = bool (*)(void *runtime, const void *action);

    blusys::framework::route_sink  *route_sink_   = nullptr;
    blusys::framework::feedback_bus *feedback_bus_ = nullptr;
    dispatch_fn                     dispatch_fn_   = nullptr;
    void                           *runtime_ptr_   = nullptr;
    void                           *product_state_ = nullptr;
#ifdef BLUSYS_FRAMEWORK_HAS_UI
    view::screen_router            *screen_router_ = nullptr;
    view::shell                    *shell_          = nullptr;
#endif
    connectivity_capability        *connectivity_   = nullptr;
    storage_capability             *storage_        = nullptr;
    bluetooth_capability           *bluetooth_      = nullptr;
    ota_capability                 *ota_            = nullptr;
    diagnostics_capability         *diagnostics_    = nullptr;
    telemetry_capability           *telemetry_      = nullptr;
    provisioning_capability        *provisioning_   = nullptr;
};

}  // namespace blusys::app
