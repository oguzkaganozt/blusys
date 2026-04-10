#pragma once

#include "blusys/framework/core/feedback.hpp"
#include "blusys/framework/core/router.hpp"

#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
namespace blusys::app::view {
class overlay_manager;
class screen_router;
struct shell;
}
#endif

#ifdef ESP_PLATFORM
// Forward declarations for C service handle types used by bundle accessors.
struct blusys_fs;
typedef struct blusys_fs blusys_fs_t;
struct blusys_fatfs;
typedef struct blusys_fatfs blusys_fatfs_t;
#endif

namespace blusys::app {

class app_runtime_base;

// Forward declarations for capability status types.
struct connectivity_status;
struct storage_status;
class connectivity_capability;
class storage_capability;

class app_ctx {
public:
    // ---- navigation (wraps framework route helpers) ----

    void navigate_to(std::uint32_t route_id, const void *params = nullptr);
    void navigate_push(std::uint32_t route_id, const void *params = nullptr);
    void navigate_replace(std::uint32_t route_id, const void *params = nullptr);
    void navigate_back();
    void show_overlay(std::uint32_t overlay_id, const void *params = nullptr);
    void hide_overlay(std::uint32_t overlay_id);

    // ---- feedback ----

    void emit_feedback(blusys::framework::feedback_channel channel,
                       blusys::framework::feedback_pattern pattern,
                       std::uint32_t value = 0);

    // ---- action dispatch (type-erased) ----

    template <typename Action>
    void dispatch(const Action &action)
    {
        if (dispatch_fn_ != nullptr) {
            dispatch_fn_(runtime_ptr_, static_cast<const void *>(&action));
        }
    }

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    // ---- screen router access (for registering screens in on_init) ----
    [[nodiscard]] view::screen_router *screen_router() const { return screen_router_; }

    // ---- overlay manager access (delegates to the screen_router's overlay_manager) ----
    [[nodiscard]] view::overlay_manager *overlay_manager() const;

    // ---- interaction shell access (nullptr if no shell configured) ----
    [[nodiscard]] view::shell *shell() const { return shell_; }
#endif

    // ---- bundle status queries ----

    [[nodiscard]] const connectivity_status *connectivity() const;
    [[nodiscard]] const storage_status *storage() const;

#ifdef ESP_PLATFORM
    // Direct storage handle access — nullptr if the bundle is absent
    // or the filesystem was not configured / failed to mount.
    [[nodiscard]] blusys_fs_t    *spiffs() const;
    [[nodiscard]] blusys_fatfs_t *fatfs()  const;
#endif

private:
    friend class app_runtime_base;

    using dispatch_fn = void (*)(void *runtime, const void *action);

    blusys::framework::route_sink  *route_sink_   = nullptr;
    blusys::framework::feedback_bus *feedback_bus_ = nullptr;
    dispatch_fn                     dispatch_fn_   = nullptr;
    void                           *runtime_ptr_   = nullptr;
#ifdef BLUSYS_FRAMEWORK_HAS_UI
    view::screen_router            *screen_router_ = nullptr;
    view::shell                    *shell_          = nullptr;
#endif
    connectivity_capability        *connectivity_   = nullptr;
    storage_capability             *storage_        = nullptr;
};

}  // namespace blusys::app
