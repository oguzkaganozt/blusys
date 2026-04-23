# Timer

General-purpose timer with microsecond resolution. Supports periodic and one-shot modes with an ISR callback.

One-shot timers stop themselves after the alarm fires.

> **API reference:** `components/blusys/include/blusys/hal/timer.h` and the generated API reference.

## Quick Example

```c
#include <stdbool.h>
#include "blusys/blusys.h"

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

## Common Mistakes

- blocking inside the callback
- changing the period or callback while the timer is running
- calling normal blocking APIs on the same timer handle from the callback

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread Safety

- concurrent calls on the same handle are serialized internally
- the callback runs outside the handle lock in ISR context
- do not call `blusys_timer_close()` concurrently with other calls on the same handle
- period and callback can only be changed while the timer is stopped

## ISR Notes

- callbacks run in ISR context
- the callback must not block
- the callback must not call normal blocking Blusys APIs
- if the callback wakes a higher-priority task, return `true`
- if `CONFIG_GPTIMER_ISR_CACHE_SAFE` is enabled, callback code must follow the ESP-IDF IRAM-safe callback rules

## Limitations

- timer resolution is fixed to microseconds; GPTimer clock selection is not exposed
- one callback can be registered per handle
- callbacks can only be changed while the timer is stopped

## Example App

See `examples/reference/hal` (timer scenario).
