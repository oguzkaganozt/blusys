# Timer

## Purpose

The `timer` module provides a handle-based API for the common general-purpose timer path:

- open one timer as periodic or one-shot
- start and stop the timer
- change the timer period while stopped
- register one direct ISR callback
- close the timer handle

The public API keeps GPTimer configuration details internal and exposes the common microsecond-based path used by application code.

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
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

## Lifecycle Model

Timer is handle-based:

1. call `blusys_timer_open()`
2. call `blusys_timer_set_callback()` while the timer is stopped if you want async notification
3. call `blusys_timer_start()` to arm the timer
4. call `blusys_timer_stop()` to disarm the timer
5. call `blusys_timer_set_period()` while stopped when you need a new period
6. call `blusys_timer_close()` when finished

`blusys_timer_close()` stops a running timer internally before releasing backend resources.

## Blocking APIs

- `blusys_timer_open()`
- `blusys_timer_close()`
- `blusys_timer_start()`
- `blusys_timer_stop()`
- `blusys_timer_set_period()`
- `blusys_timer_set_callback()`

## Async APIs

- `blusys_timer_set_callback()` registers one callback that runs directly in GPTimer ISR context

The callback return value is passed back to the GPTimer driver so application code can request a yield when it wakes a higher-priority task from ISR context.

## Thread Safety

- concurrent calls using the same handle are serialized internally
- the callback itself runs outside that handle lock in ISR context
- do not call `blusys_timer_close()` concurrently with other calls using the same handle
- do not change period or callback while the timer is running

## ISR Notes

- timer callbacks run in ISR context
- the callback must not block
- the callback must not call normal blocking Blusys APIs on the same timer handle
- if the callback wakes a higher-priority task, return `true`
- if `CONFIG_GPTIMER_ISR_CACHE_SAFE` is enabled, callback code must follow the ESP-IDF IRAM-safe callback rules

## Limitations

- the public API is fixed to a microsecond timer resolution and does not expose GPTimer clock selection
- one callback can be registered per timer handle
- callbacks can only be changed while the timer is stopped
- one-shot timers stop themselves after the alarm fires

## Error Behavior

- invalid pointers or a zero period return `BLUSYS_ERR_INVALID_ARG`
- starting an already running timer returns `BLUSYS_ERR_INVALID_STATE`
- stopping an already stopped timer returns `BLUSYS_ERR_INVALID_STATE`
- changing the period or callback while the timer is running returns `BLUSYS_ERR_INVALID_STATE`
- backend timer allocation or control failures are translated into `blusys_err_t`

## Example App

See `examples/timer_basic/`.
