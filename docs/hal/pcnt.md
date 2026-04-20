# PCNT

Pulse counter input with edge selection, watch-point callbacks, and glitch filtering.

Watch points and callbacks can only be changed while the counter is stopped.

## Quick Example

```c
#include <stdbool.h>
#include "blusys/blusys.h"

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

See `examples/validation/hal_io_lab` (PCNT scenario).
