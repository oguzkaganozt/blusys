# Run A Timer Callback

## Problem Statement

You want to start a periodic or one-shot timer and receive a callback when it fires.

## Prerequisites

- a supported board
- a task or flag update path that is safe to trigger from ISR context

## Minimal Example

```c
#include <stdbool.h>

#include "blusys/timer.h"

static bool on_tick(blusys_timer_t *timer, void *user_ctx)
{
    (void) timer;
    (void) user_ctx;
    return false;
}

void app_main(void)
{
    blusys_timer_t *timer;

    if (blusys_timer_open(1000000u, true, &timer) != BLUSYS_OK) {
        return;
    }

    blusys_timer_set_callback(timer, on_tick, NULL);
    blusys_timer_start(timer);
}
```

## APIs Used

- `blusys_timer_open()` creates a one-shot or periodic timer
- `blusys_timer_set_callback()` registers one ISR-context callback
- `blusys_timer_start()` arms the timer
- `blusys_timer_stop()` disarms it
- `blusys_timer_close()` releases the timer handle

## Expected Runtime Behavior

- the callback runs after the configured period
- periodic timers continue firing until stopped
- one-shot timers fire once and stop themselves

## Common Mistakes

- blocking inside the callback
- changing the period or callback while the timer is running
- calling normal blocking APIs on the same timer handle from the callback

## Example App

See `examples/timer_basic/`.


## API Reference

For full type definitions and function signatures, see [Timer API Reference](../modules/timer.md).
