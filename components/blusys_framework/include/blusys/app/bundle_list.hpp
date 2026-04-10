#pragma once

#include "blusys/app/bundle.hpp"

#include <cstddef>
#include <initializer_list>

namespace blusys::app {

static constexpr std::size_t kMaxBundles = 4;

struct bundle_list {
    bundle_base *items[kMaxBundles]{};
    std::size_t count = 0;

    bundle_list() = default;

    bundle_list(std::initializer_list<bundle_base *> init)
    {
        for (auto *b : init) {
            if (count < kMaxBundles) {
                items[count++] = b;
            }
        }
    }
};

}  // namespace blusys::app
