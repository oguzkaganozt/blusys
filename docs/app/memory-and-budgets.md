# Memory and budgets

All bounded resources are declared in `components/blusys/include/blusys/observe/budget.hpp`. This page describes each budget, its rationale, and what crossing it means.

## Runtime queues

| Resource | Constant | Value | Overflow policy |
|----------|----------|------:|-----------------|
| Action queue depth | `blusys::budget::action_queue_depth` | 16 | Fail closed — `BLUSYS_ERR_BUSY`, caller retries |
| Event queue depth | `blusys::budget::event_queue_depth` | 16 | Fail closed — `BLUSYS_ERR_BUSY`, dropped event logged |

The reducer drains both queues every step. Depth bounds the peak a single tick can absorb. Increase only after profiling; the current value handles every stock use case.

## Display task

| Resource | Constant | Value |
|----------|----------|------:|
| Stack (bytes) | `blusys::budget::display_task_stack_bytes` | 16 384 |
| Priority | `blusys::budget::display_task_priority` | 5 |
| Default render buffer (scanlines) | `blusys::budget::display_buf_lines` | 20 |
| Flush band (scanlines) | `blusys::budget::display_flush_band_lines` | 40 |

The display task is the only framework task that owns an LVGL render cycle. Do not call LVGL from any other task without `blusys_display_lock`.

## Device heap

| Resource | Constant | Target |
|----------|----------|-------:|
| Free heap at steady state | `blusys::budget::device_heap_free_target_bytes` | 300 KiB |

This is a *floor*, not a limit. OTA staging, crash dump, and growth reserves sit below this line. The framework logs a domain-stamped warning when free heap drops below the target.

Host builds do not enforce this constant.

## OOM policy

Every allocation path in the framework follows fail-closed semantics:

1. Attempt allocation.
2. On failure: log a structured error (`BLUSYS_LOG(ERROR, domain, …)`), bump the domain counter, return a domain-stamped `BLUSYS_ERR_NO_MEM`.
3. Never silently return partial results or fall back to a smaller buffer.

Callers (capabilities, the reducer, UI callbacks) must propagate `BLUSYS_ERR_NO_MEM` or handle it explicitly — not swallow it.

## Changing a budget

A budget change is a policy change:

1. Update the constant in `budget.hpp`.
2. Update the table in this file.
3. Reference: `docs/app/memory-and-budgets.md` is cited by `budget.hpp`; keep both in sync.

## See also

- `components/blusys/include/blusys/observe/budget.hpp` — source of truth
- [Threading contract](../internals/threading.md) — task ownership, ISR constraints, queue lifetime
