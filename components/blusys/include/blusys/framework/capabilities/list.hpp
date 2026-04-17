#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <cstddef>

namespace blusys {

// Non-owning view over an array of capability pointers. `app_spec` holds a
// `capability_list*`; storage is owned by the product, typically as a
// `capability_list_storage<N>` (below) at static storage duration.
struct capability_list {
    capability_base* const* items = nullptr;
    std::size_t              count = 0;

    constexpr capability_list() noexcept = default;
    constexpr capability_list(capability_base* const* i, std::size_t n) noexcept
        : items(i), count(n) {}
};

// Storage + view, sized from the constructor arguments. Preferred way to
// declare a product's capability set:
//
//   static blusys::capability_list_storage capabilities{&conn, &mqtt, &ota};
//   spec.capabilities = &capabilities;  // implicit upcast to capability_list*
//
// Marked `final` so the missing virtual destructor in the view base is not
// a slicing hazard. Copy/move are deleted — the base `capability_list`
// would continue pointing at the moved-from `slots` array, which would be
// a dangling-pointer bug.
template <std::size_t N>
struct capability_list_storage final : capability_list {
    capability_base* slots[N];

    template <typename... Caps>
    constexpr explicit capability_list_storage(Caps*... caps) noexcept
        // Base-then-member init order is declaration order, not written
        // order: `capability_list{slots, N}` captures the address of the
        // `slots` subobject (memory already allocated for the derived
        // object), then `slots{caps...}` populates it. Taking the address
        // of an uninitialized array member is well-defined.
        : capability_list{slots, N}, slots{caps...}
    {
        // Guards against explicit mismatched N (e.g. capability_list_storage<3>{&a, &b}).
        // Bypassed when CTAD deduces N from the argument pack.
        static_assert(sizeof...(Caps) == N,
                      "capability_list_storage: argument count != N");
    }

    capability_list_storage(const capability_list_storage &)            = delete;
    capability_list_storage &operator=(const capability_list_storage &) = delete;
    capability_list_storage(capability_list_storage &&)                 = delete;
    capability_list_storage &operator=(capability_list_storage &&)      = delete;
};

template <typename... Caps>
capability_list_storage(Caps*... caps) -> capability_list_storage<sizeof...(Caps)>;

// Equivalent to `capability_list_storage<sizeof...(Caps)>{caps...}`. Prefer
// direct construction with CTAD; this helper exists for parity with other
// `make_` factories and for use inside templates where CTAD is awkward.
template <typename... Caps>
constexpr capability_list_storage<sizeof...(Caps)>
make_capability_list(Caps*... caps) noexcept
{
    return capability_list_storage<sizeof...(Caps)>{caps...};
}

}  // namespace blusys
