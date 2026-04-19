#pragma once

#include "blusys/framework/app/fx.hpp"
#include "blusys/framework/capabilities/list.hpp"
#include "blusys/framework/feedback/feedback.hpp"

#include "blusys/hal/error.h"

#include <cassert>
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace blusys {

class app_runtime_base;

class app_ctx {
public:
    [[nodiscard]] app_fx &fx() noexcept
    {
        return fx_;
    }

    [[nodiscard]] const app_fx &fx() const noexcept
    {
        return fx_;
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
        constexpr std::size_t kind_index = capability_kind_index(T::kind_value);
        static_assert(kind_index < kCapabilityKindCount,
                      "ctx.get<T>: capability kind out of range");

        capability_base *c = capabilities_by_kind_[kind_index];
        return c != nullptr ? static_cast<T *>(c) : nullptr;
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

private:
    friend class app_runtime_base;

    void bind_capabilities(capability_list *capabilities) noexcept
    {
        capabilities_ = capabilities;
        capabilities_by_kind_.fill(nullptr);

        if (capabilities == nullptr || capabilities->items == nullptr) {
            return;
        }

        for (std::size_t i = 0; i < capabilities->count; ++i) {
            capability_base *c = capabilities->items[i];
            if (c == nullptr) {
                continue;
            }

            const std::size_t kind_index = capability_kind_index(c->kind());
            if (kind_index < capabilities_by_kind_.size() &&
                capabilities_by_kind_[kind_index] == nullptr) {
                capabilities_by_kind_[kind_index] = c;
            }
        }
    }

    std::uint32_t action_queue_drop_count_ = 0;

    using dispatch_fn = bool (*)(void *runtime, const void *action);

    blusys::feedback_bus *feedback_bus_  = nullptr;
    dispatch_fn           dispatch_fn_   = nullptr;
    void                 *runtime_ptr_   = nullptr;
    void                 *product_state_ = nullptr;
    app_fx                fx_            = {};

    // Non-owning view of the product's composed capability list. Set by the
    // runtime during init; drives `get<T>()` / `status_of<T>()`.
    capability_list *capabilities_ = nullptr;

    // Dense O(1) lookup table keyed by capability_kind.
    std::array<capability_base *, kCapabilityKindCount> capabilities_by_kind_{};
};

}  // namespace blusys
