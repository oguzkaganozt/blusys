# Memory and budgets

Blusys keeps bounded resources explicit. The constants live in `components/blusys/include/blusys/observe/budget.hpp`.

## Runtime queues

| Resource | Constant | Value | Policy |
|----------|----------|------:|--------|
| Action queue depth | `blusys::budget::action_queue_depth` | 16 | fail closed |
| Event queue depth | `blusys::budget::event_queue_depth` | 16 | fail closed |

## Display task

| Resource | Constant | Value |
|----------|----------|------:|
| Stack | `blusys::budget::display_task_stack_bytes` | 16 384 |
| Priority | `blusys::budget::display_task_priority` | 5 |
| Render buffer | `blusys::budget::display_buf_lines` | 20 |
| Flush band | `blusys::budget::display_flush_band_lines` | 40 |

## Heap target

| Resource | Constant | Target |
|----------|----------|-------:|
| Free heap at steady state | `blusys::budget::device_heap_free_target_bytes` | 300 KiB |

The value is a floor, not a limit.

## OOM policy

1. Try the allocation.
2. Log a structured error on failure.
3. Return `BLUSYS_ERR_NO_MEM`.
4. Never silently shrink or partially succeed.

## Changing a budget

Update the constant, update this page, and keep the source comment in sync.

## See also

- `components/blusys/include/blusys/observe/budget.hpp`
- [Threading contract](../internals/threading.md)
