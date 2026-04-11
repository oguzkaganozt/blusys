#pragma once

#include <cstdint>

namespace blusys_examples {

/// Clamp a value to [0, 100]. Shared by Phase 5 interactive archetype reducers and host smoke tests.
inline std::int32_t clamp_percent(std::int32_t value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

}  // namespace blusys_examples
