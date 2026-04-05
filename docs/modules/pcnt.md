# PCNT

## Purpose

The `pcnt` module provides a handle-based API for the common pulse counting path:

- open one pulse counter input
- count rising, falling, or both edges
- start and stop counting
- clear and read the current count
- register one watch-point callback

The public API keeps the ESP-IDF PCNT unit and channel model internal and exposes a smaller edge-counting surface.

## Supported Targets

- ESP32
- ESP32-S3

On `ESP32-C3`, the public API is present but returns `BLUSYS_ERR_NOT_SUPPORTED`, and `blusys_target_supports(BLUSYS_FEATURE_PCNT)` reports `false`.

## Quick Start Example

```c
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

## Lifecycle Model

PCNT is handle-based:

1. call `blusys_pcnt_open()`
2. optionally call `blusys_pcnt_set_callback()` while stopped
3. optionally call `blusys_pcnt_add_watch_point()` while stopped
4. call `blusys_pcnt_start()` to begin counting
5. call `blusys_pcnt_stop()` before reconfiguration or close
6. call `blusys_pcnt_close()` when finished

## Blocking APIs

- `blusys_pcnt_open()`
- `blusys_pcnt_close()`
- `blusys_pcnt_start()`
- `blusys_pcnt_stop()`
- `blusys_pcnt_clear_count()`
- `blusys_pcnt_get_count()`
- `blusys_pcnt_add_watch_point()`
- `blusys_pcnt_remove_watch_point()`
- `blusys_pcnt_set_callback()`

## Async APIs

- `blusys_pcnt_set_callback()` registers one ISR-context callback

The callback runs when the counter reaches any configured watch point. The return value is passed back to the PCNT driver so application code can request a yield after waking a higher-priority task from ISR context.

## Thread Safety

- concurrent calls using the same handle are serialized internally
- the callback runs outside that handle lock in ISR context
- do not call `blusys_pcnt_close()` concurrently with other calls using the same handle
- watch points and callbacks can only be changed while the counter is stopped

## ISR Notes

- watch-point callbacks run in ISR context
- the callback must not block
- use only ISR-safe FreeRTOS APIs from the callback
- if the callback wakes a higher-priority task, return `true`

## Limitations

- the public API is limited to one input pin per handle
- only watch-point callbacks are exposed publicly
- the count range is limited to the internal hardware range used by the module
- quadrature and control-signal modes are not exposed yet

## Error Behavior

- invalid pointers, invalid pins, and out-of-range watch points return `BLUSYS_ERR_INVALID_ARG`
- starting an already running counter returns `BLUSYS_ERR_INVALID_STATE`
- stopping an already stopped counter returns `BLUSYS_ERR_INVALID_STATE`
- changing callbacks or watch points while running returns `BLUSYS_ERR_INVALID_STATE`
- backend allocation and control failures are translated into `blusys_err_t`

## Example App

See `examples/pcnt_basic/`.
