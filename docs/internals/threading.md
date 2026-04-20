# Threading contract

This document states the threading rules that every subsystem and capability in
Blusys must obey. Violations are bugs, not styles — the runtime is designed
around a small, deliberate set of task boundaries, and code that ignores them
will corrupt state, miss events, or leak allocations under load.

## Summary

- The **reducer** is the single owner of app state. It runs synchronously on
  whichever task invokes `app_runtime::step()` — today that is the caller's
  task, typically `app_main` or the display render task.
- **Effect handlers** run on the reducer's task unless they are explicitly
  marked async. Async handlers must not touch app state directly; they hand
  results back through the event queue.
- **Capability `poll()`** is cooperative: it runs on the reducer task, does
  not block, and uses `post_event()` to feed work back into the queue.
- **ISR → task handoff** uses FreeRTOS task notifications
  (`vTaskNotifyGiveFromISR` + `ulTaskNotifyTake`) as the canonical primitive.
  ISRs never call framework or capability code directly.

## Tasks owned by the framework

| Task | Created at | Stack | Priority | Runs |
|---|---|---:|---:|---|
| App / reducer | app entry (caller) | — | — | `app_runtime::step()`, reducer, effect handlers, capability `poll()` |
| Display render | `blusys_display_open()` | 16 KiB | 5 | LVGL timer tick, flush callback, panel blit |
| UART events | `blusys_uart_open()` | 4 KiB | IDF default | UART driver event callback; enqueues to reducer |
| WebSocket recv | `blusys_ws_client_open()` | 4 KiB | `tskIDLE_PRIORITY + 2` | Decodes incoming frames; enqueues to reducer |

Stack and priority defaults are declared in
`include/blusys/observe/budget.hpp` and must not be overridden
silently — any subsystem that needs a different value passes it through its
`*_config_t` and documents the reason.

## Queues

| Queue | Owner | Depth | Overflow |
|---|---|---:|---|
| Action queue | `app_runtime` | `blusys::budget::action_queue_depth` (16) | Fail closed: returns `BLUSYS_ERR_BUSY`, logs to `framework.runtime` |
| Event queue | `runtime` (engine) | `blusys::budget::event_queue_depth` (16) | Fail closed: returns `BLUSYS_ERR_BUSY`, logs to `framework.events` |

A full queue is never silently dropped. Producers either back off or surface
the failure through their domain-stamped error.

## OOM policy

Every allocation has an owner and a fail-closed path:

1. Log a structured line with domain + site + bytes requested.
2. Bump the domain counter (`blusys_counter_inc`).
3. Return a domain-stamped `blusys_err_t` with `BLUSYS_ERR_NO_MEM` in the low
   16 bits.

The `display.c` and storage paths are the canonical examples; new subsystems
copy that shape.

## ISR constraints

Code running in an ISR context may only:

- Call the FreeRTOS `*FromISR` APIs.
- Bump counters via `blusys_counter_inc` (atomic, relaxed).
- Post to a task notification.

It must not:

- Call `blusys_log`, `BLUSYS_LOG*`, or any formatting path.
- Take locks, allocate, or call capability / reducer code.
- Access app state in any form.

The HAL peripherals that expose ISR-adjacent callbacks document this contract
at their public API surface.

## Verification

- `ctx.fx()` is the approved navigation/storage surface; routing must stay off the thread-safety hot path.
- `#ifdef ESP_PLATFORM` above the HAL line — forbidden
  (`scripts/check-no-platform-ifdef-above-hal.sh`).
- Stack / queue budgets — cross-checked against
  `observe/budget.hpp` at build time.

## Why these rules

A small set of tasks keeps the mental model tractable: any given function runs
on a named task and can reason about what state it owns. Event-queue handoff
makes cross-task communication explicit and debuggable. Task notifications are
cheap, bounded, and free of priority inversion — the right default for ISR
work. Budgets declared in one header let reviewers spot any change that
affects latency or memory without reading every driver.
