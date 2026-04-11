#pragma once

// Opt-in helper when Action is std::variant<...> — dispatch the held alternative through app_ctx.
// Typical apps use a plain enum or tagged struct; this is for products that prefer std::variant.

#include "blusys/app/app_ctx.hpp"

#include <utility>
#include <variant>

namespace blusys::app {

template <typename... Alternatives>
[[nodiscard]] inline bool dispatch_variant(app_ctx &ctx, const std::variant<Alternatives...> &action)
{
    return std::visit([&ctx](const auto &a) { return ctx.dispatch(a); }, action);
}

}  // namespace blusys::app
