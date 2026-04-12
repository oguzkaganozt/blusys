#pragma once

#include <atomic>
#include <cstdint>

namespace blusys::app::detail {

/// Atomically read and clear pending capability bits (acquire on RMW).
/// Matches `pending_flags_.exchange(cleared_state, std::memory_order_acquire)`.
[[nodiscard]] inline std::uint32_t drain_pending_flags(
    std::atomic<std::uint32_t> &pending,
    std::uint32_t cleared_state = 0U)
{
    return pending.exchange(cleared_state, std::memory_order_acquire);
}

}  // namespace blusys::app::detail
