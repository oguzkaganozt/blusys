# Threading contract

The reducer and every capability must obey the same task boundaries.

## At a glance

- the reducer runs on the task that calls `app_runtime::step()`
- capabilities poll cooperatively on that task
- ISRs only hand work off to tasks
- queues are bounded and fail closed

## Owned tasks

| Task | Runs |
|------|------|
| App / reducer | `app_runtime::step()`, reducer, effect handlers, capability `poll()` |
| Display render | LVGL tick, flush callback, panel blit |
| UART events | UART driver callback, then queue handoff |
| WebSocket recv | frame decode, then queue handoff |

## Queues

| Queue | Depth | Overflow |
|-------|------:|----------|
| Action queue | 16 | `BLUSYS_ERR_BUSY`, logged |
| Event queue | 16 | `BLUSYS_ERR_BUSY`, logged |

## OOM policy

1. Log the allocation failure.
2. Bump the domain counter.
3. Return `BLUSYS_ERR_NO_MEM`.

## ISR constraints

Allowed in ISR context:

- FreeRTOS `*FromISR` APIs
- `blusys_counter_inc`
- task notifications

Not allowed in ISR context:

- logging or formatting
- locks or allocation
- reducer or capability code

## Verification

- `ctx.fx()` is the approved navigation and storage surface.
- Stack and queue budgets live in `observe/budget.hpp`.
- `#ifdef ESP_PLATFORM` stays below the HAL line.

## Why these rules

The model keeps ownership obvious, makes cross-task communication explicit, and prevents hidden queue growth or accidental state access from interrupts.
