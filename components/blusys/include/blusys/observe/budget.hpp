/**
 * @file budget.hpp
 * @brief Per-platform budgets — the single source of truth for bounded resources.
 *
 * Every bounded resource on the platform (queue depth, task stack, scratch
 * buffer size, heap reservation) is declared here so the compiler, the
 * runtime, and docs all see the same numbers. A budget is a *ceiling*:
 * crossing it should fail closed with a domain-stamped error, not an implicit
 * fallback.
 *
 * Contract:
 *   1. Anything that allocates or bounds a queue references a constant from
 *      this header. No magic numbers elsewhere.
 *   2. Values are pinned per platform (device vs host). Where both compile
 *      identically they share one value.
 *   3. Changing a budget is a policy change; it requires updating
 *      docs/app/memory-and-budgets.md alongside the header.
 *
 * This header is C++ because the framework reducer and queues are C++. C code
 * that needs the same numbers should keep them local to its subsystem and
 * cite this header by name.
 */

#ifndef BLUSYS_OBSERVE_BUDGET_HPP
#define BLUSYS_OBSERVE_BUDGET_HPP

#include <cstddef>
#include <cstdint>

namespace blusys::budget {

/** @brief Maximum actions in flight between reducer dispatches. */
inline constexpr std::size_t action_queue_depth = 16;

/** @brief Maximum framework events queued before the reducer runs. */
inline constexpr std::size_t event_queue_depth = 16;

/** @brief Display render task stack size, in bytes. */
inline constexpr std::uint32_t display_task_stack_bytes = 16384;

/** @brief Display render task FreeRTOS priority. */
inline constexpr int           display_task_priority    = 5;

/** @brief Default LVGL double-buffer sizing, in scanlines (consulted by display.c when a config omits an explicit value). */
inline constexpr std::uint32_t display_buf_lines        = 20;

/** @brief Default LVGL flush-band height, in scanlines. */
inline constexpr std::uint32_t display_flush_band_lines = 40;

/** @brief Target free heap after the app reaches steady state (device only; host does not enforce). */
inline constexpr std::size_t device_heap_free_target_bytes = 300 * 1024;

}  // namespace blusys::budget

#endif  // BLUSYS_OBSERVE_BUDGET_HPP
