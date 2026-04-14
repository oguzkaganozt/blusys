#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <cstddef>
#include <initializer_list>

namespace blusys {

static constexpr std::size_t kMaxCapabilities = 8;

struct capability_list {
    capability_base *items[kMaxCapabilities]{};
    std::size_t count = 0;

    capability_list() = default;

    capability_list(std::initializer_list<capability_base *> init)
    {
        for (auto *c : init) {
            if (count < kMaxCapabilities) {
                items[count++] = c;
            }
        }
    }
};

}  // namespace blusys
