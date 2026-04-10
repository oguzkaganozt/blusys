#pragma once

#include "blusys/error.h"

#include <cstdint>

namespace blusys::framework { class runtime; }

namespace blusys::app {

enum class bundle_kind : std::uint8_t {
    connectivity,
    storage,
    custom,
};

class bundle_base {
public:
    virtual ~bundle_base() = default;

    [[nodiscard]] virtual bundle_kind kind() const = 0;

    virtual blusys_err_t start(blusys::framework::runtime &rt) = 0;
    virtual void poll(std::uint32_t now_ms) = 0;
    virtual void stop() = 0;
};

}  // namespace blusys::app
