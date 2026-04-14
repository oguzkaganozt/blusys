#pragma once

#include "blusys/framework/app/services.hpp"
#include "blusys/framework/feedback/feedback.hpp"

#include "blusys/hal/error.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace blusys {

class app_runtime_base;
struct capability_list;

// Forward declarations for capability status types.
struct connectivity_status;
struct storage_status;
struct bluetooth_status;
struct ota_status;
struct diagnostics_status;
struct telemetry_status;
struct lan_control_status;
struct usb_status;
class connectivity_capability;
class storage_capability;
class bluetooth_capability;
class ota_capability;
class diagnostics_capability;
class telemetry_capability;
class lan_control_capability;
class usb_capability;
class mqtt_host_capability;

class app_ctx {
public:
    // ---- routing / UI / FS (framework services; not part of reducer-facing state) ----

    [[nodiscard]] app_services &services() noexcept
    {
        return *services_;
    }

    [[nodiscard]] const app_services &services() const noexcept
    {
        return *services_;
    }

    // ---- feedback ----

    void emit_feedback(blusys::feedback_channel channel,
                       blusys::feedback_pattern pattern,
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

    // Count of actions dropped because the fixed-size action queue was full (dispatch
    // returned false). Monotonic for the lifetime of this ctx — rebinding a runtime
    // does not reset it. Query from update(), on_tick, or diagnostics for overload
    // debugging.
    [[nodiscard]] std::uint32_t action_queue_drop_count() const noexcept
    {
        return action_queue_drop_count_;
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

    // ---- capability status queries ----

    [[nodiscard]] const connectivity_status *connectivity() const;
    [[nodiscard]] const storage_status *storage() const;
    [[nodiscard]] const bluetooth_status *bluetooth() const;
    [[nodiscard]] const ota_status *ota() const;
    [[nodiscard]] const diagnostics_status *diagnostics() const;
    [[nodiscard]] const telemetry_status *telemetry() const;
    [[nodiscard]] const lan_control_status *lan_control() const;
    [[nodiscard]] const usb_status *usb() const;

    // Host (SDL) MQTT client — nullptr when capability not registered or on ESP-IDF product builds.
    [[nodiscard]] mqtt_host_capability *mqtt_host();

    // ---- capability requests (integration calls from intent bridges) ----
    // These forward to the composed capability when present; otherwise return
    // BLUSYS_ERR_INVALID_STATE.
    blusys_err_t request_connectivity_reconnect();

private:
    friend class app_runtime_base;

    static void bind_capability_pointers_from_list(app_ctx &ctx, capability_list *capabilities);

    std::uint32_t action_queue_drop_count_ = 0;

    using dispatch_fn = bool (*)(void *runtime, const void *action);

    blusys::feedback_bus *feedback_bus_ = nullptr;
    dispatch_fn                        dispatch_fn_  = nullptr;
    void                              *runtime_ptr_  = nullptr;
    void                              *product_state_ = nullptr;
    app_services                      *services_      = nullptr;

    connectivity_capability *connectivity_  = nullptr;
    storage_capability      *storage_       = nullptr;
    bluetooth_capability    *bluetooth_     = nullptr;
    ota_capability            *ota_         = nullptr;
    diagnostics_capability   *diagnostics_  = nullptr;
    telemetry_capability     *telemetry_    = nullptr;
    lan_control_capability   *lan_control_  = nullptr;
    usb_capability           *usb_          = nullptr;
    mqtt_host_capability      *mqtt_host_   = nullptr;
};

}  // namespace blusys
