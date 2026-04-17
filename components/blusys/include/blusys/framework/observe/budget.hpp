/* blusys/framework/observe/budget.hpp — per-platform budgets.
 *
 * Every bounded resource on the platform (queue depth, task stack, scratch
 * buffer size, heap reservation) is declared here so the compiler, the runtime
 * and docs all see the same numbers. A budget is a *ceiling*: crossing it
 * should fail closed with a domain-stamped error, not an implicit fallback.
 *
 * The contract:
 *   1. Anything that allocates or bounds a queue should reference a constant
 *      from this header. No magic numbers elsewhere.
 *   2. Values are pinned per platform (device vs host). Where both compile
 *      identically they share one value.
 *   3. Changing a budget is a policy change; it requires updating
 *      docs/app/memory-and-budgets.md (P9) alongside the header.
 *
 * This header is C++ because the framework reducer + queues are C++. C code
 * that needs the same numbers should keep them local to its subsystem and
 * cite this header by name.
 */

#ifndef BLUSYS_FRAMEWORK_OBSERVE_BUDGET_HPP
#define BLUSYS_FRAMEWORK_OBSERVE_BUDGET_HPP

#include <cstddef>
#include <cstdint>

namespace blusys::budget {

/* ── Runtime queues ───────────────────────────────────────────────────────── */

/* Maximum actions in flight between dispatches. The reducer drains the queue
 * every step; depth bounds the peak a single tick can absorb. */
inline constexpr std::size_t action_queue_depth = 16;

/* Maximum framework events queued before the reducer runs. Same shape as the
 * action queue; kept separate so the two can be tuned independently. */
inline constexpr std::size_t event_queue_depth = 16;

/* ── Display render task ─────────────────────────────────────────────────── */

inline constexpr std::uint32_t display_task_stack_bytes = 16384;
inline constexpr int           display_task_priority    = 5;

/* Default LVGL double-buffer sizing, in scanlines. Consulted by display.c
 * when a config omits an explicit value. */
inline constexpr std::uint32_t display_buf_lines        = 20;
inline constexpr std::uint32_t display_flush_band_lines = 40;

/* ── Heap budget (device) ─────────────────────────────────────────────────── */

/* Target free heap after the app reaches steady state. OTA, crash dump and
 * growth reserves sit below this line; we prefer failing fast to silently
 * eating into them. Host builds do not enforce this. */
inline constexpr std::size_t device_heap_free_target_bytes = 300 * 1024;

}  // namespace blusys::budget

#endif  // BLUSYS_FRAMEWORK_OBSERVE_BUDGET_HPP
