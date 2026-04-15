# Timer

General-purpose timer with microsecond resolution. Supports periodic and one-shot modes with an ISR callback.

## Quick Example

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

## Common Mistakes

- blocking inside the callback
- changing the period or callback while the timer is running
- calling normal blocking APIs on the same timer handle from the callback

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_timer_t`

```c
typedef struct blusys_timer blusys_timer_t;
```

Opaque handle returned by `blusys_timer_open()`.

### `blusys_timer_callback_t`

```c
typedef bool (*blusys_timer_callback_t)(blusys_timer_t *timer, void *user_ctx);
```

Called directly in GPTimer ISR context when the alarm fires. Return `true` to request a FreeRTOS yield after waking a higher-priority task.

## Functions

### `blusys_timer_open`

```c
blusys_err_t blusys_timer_open(uint32_t period_us, bool periodic, blusys_timer_t **out_timer);
```

Allocates a GPTimer with the given period. The timer is not started until `blusys_timer_start()` is called.

**Parameters:**
- `period_us` — alarm period in microseconds; must be greater than zero
- `periodic` — `true` for repeating, `false` for one-shot
- `out_timer` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for zero period or NULL pointer.

---

### `blusys_timer_close`

```c
blusys_err_t blusys_timer_close(blusys_timer_t *timer);
```

Stops the timer if running, then releases the handle.

---

### `blusys_timer_start`

```c
blusys_err_t blusys_timer_start(blusys_timer_t *timer);
```

Arms the timer. The callback fires after each period expires.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if already running.

---

### `blusys_timer_stop`

```c
blusys_err_t blusys_timer_stop(blusys_timer_t *timer);
```

Disarms the timer. The timer may be reconfigured and restarted.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if already stopped.

---

### `blusys_timer_set_period`

```c
blusys_err_t blusys_timer_set_period(blusys_timer_t *timer, uint32_t period_us);
```

Changes the alarm period. Must be called while the timer is stopped.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if the timer is running.

---

### `blusys_timer_set_callback`

```c
blusys_err_t blusys_timer_set_callback(blusys_timer_t *timer,
                                       blusys_timer_callback_t callback,
                                       void *user_ctx);
```

Registers or replaces the ISR callback. Must be called while the timer is stopped.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if the timer is running.

## Lifecycle

1. `blusys_timer_open()` — allocate timer
2. `blusys_timer_set_callback()` — register ISR callback (while stopped)
3. `blusys_timer_start()` — arm
4. `blusys_timer_stop()` — disarm (optional, for reconfiguration)
5. `blusys_timer_close()` — release

One-shot timers stop themselves after the alarm fires.

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
