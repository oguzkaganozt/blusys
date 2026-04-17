#pragma once

#include "blusys/framework/app/services.hpp"
#include "blusys/framework/capabilities/list.hpp"
#include "blusys/framework/feedback/feedback.hpp"

#include "blusys/hal/error.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace blusys {

class app_runtime_base;

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

    // ---- capability lookup ----
    //
    // Returns a pointer to the composed capability of type T, or nullptr if
    // no capability of that kind is in the product's `capability_list`.
    // T must expose `static constexpr capability_kind kind_value`.
    //
    //   auto *mqtt = ctx.get<blusys::mqtt_capability>();
    //   if (mqtt != nullptr) { mqtt->publish(...); }
    template <typename T>
    [[nodiscard]] T *get() const noexcept
    {
        static_assert(std::is_base_of_v<capability_base, T>,
                      "ctx.get<T>: T must derive from capability_base");
        if (capabilities_ == nullptr) {
            return nullptr;
        }
        for (std::size_t i = 0; i < capabilities_->count; ++i) {
            capability_base *c = capabilities_->items[i];
            if (c != nullptr && c->kind() == T::kind_value) {
                return static_cast<T *>(c);
            }
        }
        return nullptr;
    }

    // Convenience: returns `&capability->status()` or nullptr when the
    // capability is not composed.
    //
    //   if (const auto *s = ctx.status_of<connectivity_capability>();
    //       s != nullptr && s->has_ip) { ... }
    template <typename T>
    [[nodiscard]] auto status_of() const noexcept
        -> decltype(&std::declval<const T>().status())
    {
        const T *c = get<T>();
        return c != nullptr ? &c->status() : nullptr;
    }

    // ---- capability requests (integration calls from intent bridges) ----
    // These forward to the composed capability when present; otherwise return
    // BLUSYS_ERR_INVALID_STATE.
    blusys_err_t request_connectivity_reconnect();

private:
    friend class app_runtime_base;

    std::uint32_t action_queue_drop_count_ = 0;

    using dispatch_fn = bool (*)(void *runtime, const void *action);

    blusys::feedback_bus *feedback_bus_  = nullptr;
    dispatch_fn           dispatch_fn_   = nullptr;
    void                 *runtime_ptr_   = nullptr;
    void                 *product_state_ = nullptr;
    app_services         *services_      = nullptr;

    // Non-owning view of the product's composed capability list. Set by the
    // runtime during init; drives `get<T>()` / `status_of<T>()`.
    capability_list *capabilities_ = nullptr;
};

}  // namespace blusys
