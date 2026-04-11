# PCNT

Pulse counter input with edge selection, watch-point callbacks, and glitch filtering.

## Quick Example

```c
#include <stdbool.h>

#include "blusys/pcnt.h"

static bool on_watch(blusys_pcnt_t *pcnt, int watch_point, void *user_ctx)
{
    (void) pcnt;
    (void) watch_point;
    (void) user_ctx;
    return false;
}

void app_main(void)
{
    blusys_pcnt_t *pcnt;

    if (blusys_pcnt_open(5, BLUSYS_PCNT_EDGE_RISING, 1000u, &pcnt) != BLUSYS_OK) {
        return;
    }

    blusys_pcnt_set_callback(pcnt, on_watch, NULL);
    blusys_pcnt_add_watch_point(pcnt, 100);
    blusys_pcnt_start(pcnt);
}
```

## Common Mistakes

- forgetting to connect the pulse source to the input pin
- doing blocking work inside the callback
- changing callbacks or watch points while the counter is running
- expecting encoder or quadrature behavior from the first `pcnt` release

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | no |
| ESP32-S3 | yes |

On ESP32-C3, all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_PCNT)` to check at runtime.

## Types

### `blusys_pcnt_t`

```c
typedef struct blusys_pcnt blusys_pcnt_t;
```

Opaque handle returned by `blusys_pcnt_open()`.

### `blusys_pcnt_edge_t`

```c
typedef enum {
    BLUSYS_PCNT_EDGE_RISING = 0,
    BLUSYS_PCNT_EDGE_FALLING,
    BLUSYS_PCNT_EDGE_BOTH,
} blusys_pcnt_edge_t;
```

### `blusys_pcnt_callback_t`

```c
typedef bool (*blusys_pcnt_callback_t)(blusys_pcnt_t *pcnt, int watch_point, void *user_ctx);
```

Called in ISR context when the counter reaches a configured watch point. `watch_point` is the threshold that triggered the event. Return `true` to request a FreeRTOS yield.

## Functions

### `blusys_pcnt_open`

```c
blusys_err_t blusys_pcnt_open(int pin,
                              blusys_pcnt_edge_t edge,
                              uint32_t max_glitch_ns,
                              blusys_pcnt_t **out_pcnt);
```

Opens a pulse counter on the given pin. Counting begins only after `blusys_pcnt_start()`.

**Parameters:**
- `pin` — GPIO number
- `edge` — edges to count
- `max_glitch_ns` — glitch filter threshold in nanoseconds; pulses shorter than this are ignored (set to 0 to disable)
- `out_pcnt` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments, `BLUSYS_ERR_NOT_SUPPORTED` on ESP32-C3.

---

### `blusys_pcnt_close`

```c
blusys_err_t blusys_pcnt_close(blusys_pcnt_t *pcnt);
```

Stops counting and releases the handle.

---

### `blusys_pcnt_start`

```c
blusys_err_t blusys_pcnt_start(blusys_pcnt_t *pcnt);
```

Enables the counter.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if already running.

---

### `blusys_pcnt_stop`

```c
blusys_err_t blusys_pcnt_stop(blusys_pcnt_t *pcnt);
```

Disables the counter without clearing the count.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if already stopped.

---

### `blusys_pcnt_clear_count`

```c
blusys_err_t blusys_pcnt_clear_count(blusys_pcnt_t *pcnt);
```

Resets the counter to zero.

---

### `blusys_pcnt_get_count`

```c
blusys_err_t blusys_pcnt_get_count(blusys_pcnt_t *pcnt, int *out_count);
```

Returns the current counter value.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `out_count` is NULL.

---

### `blusys_pcnt_add_watch_point`

```c
blusys_err_t blusys_pcnt_add_watch_point(blusys_pcnt_t *pcnt, int count);
```

Adds a watch-point threshold. The registered callback fires when the counter reaches this value. Must be called while the counter is stopped.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if the counter is running.

---

### `blusys_pcnt_remove_watch_point`

```c
blusys_err_t blusys_pcnt_remove_watch_point(blusys_pcnt_t *pcnt, int count);
```

Removes a previously added watch-point. Must be called while the counter is stopped.

---

### `blusys_pcnt_set_callback`

```c
blusys_err_t blusys_pcnt_set_callback(blusys_pcnt_t *pcnt,
                                      blusys_pcnt_callback_t callback,
                                      void *user_ctx);
```

Registers the ISR callback for watch-point events. Must be called while the counter is stopped.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if the counter is running.

## Lifecycle

1. `blusys_pcnt_open()` — configure pin and edge
2. `blusys_pcnt_set_callback()` / `blusys_pcnt_add_watch_point()` — configure events (while stopped)
3. `blusys_pcnt_start()` — begin counting
4. `blusys_pcnt_get_count()` — read count at any time
5. `blusys_pcnt_stop()` — pause (optional, for reconfiguration)
6. `blusys_pcnt_close()` — release

## Thread Safety

- concurrent calls on the same handle are serialized internally
- the callback runs outside the handle lock in ISR context
- do not call `blusys_pcnt_close()` concurrently with other calls on the same handle
- watch points and callbacks can only be changed while the counter is stopped

## ISR Notes

- watch-point callbacks run in ISR context
- the callback must not block
- use only ISR-safe FreeRTOS APIs from the callback
- if the callback wakes a higher-priority task, return `true`

## Limitations

- one input pin per handle
- only watch-point callbacks are exposed; continuous interrupt on every edge is not
- quadrature and control-signal modes are not exposed

## Example App

See `examples/validation/pcnt_basic/`.
