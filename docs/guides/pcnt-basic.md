# Count Pulses And Watch Thresholds

## Problem Statement

You want to count incoming pulses on one GPIO and get a callback when the count reaches a specific threshold.

## Prerequisites

- a supported board
- one pulse source connected to the PCNT input pin
- if you use the bundled example, jumper the PWM output pin to the PCNT input pin

This guide applies to `ESP32` and `ESP32-S3`. On `ESP32-C3`, `pcnt` is reported as unsupported.

## Minimal Example

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

## APIs Used

- `blusys_pcnt_open()` creates one pulse counter handle
- `blusys_pcnt_add_watch_point()` installs a threshold value
- `blusys_pcnt_set_callback()` registers one ISR-context callback
- `blusys_pcnt_start()` begins counting
- `blusys_pcnt_get_count()` reads the current count
- `blusys_pcnt_stop()` stops counting
- `blusys_pcnt_close()` releases the handle

## Expected Runtime Behavior

- the count increases as pulses arrive on the configured input pin
- the callback runs when the count reaches each configured watch point
- stopping the counter freezes the count value until restarted

## Common Mistakes

- forgetting to connect the pulse source to the input pin
- doing blocking work inside the callback
- changing callbacks or watch points while the counter is running
- expecting encoder or quadrature behavior from the first `pcnt` release

## Example App

See `examples/pcnt_basic/`.


## API Reference

For full type definitions and function signatures, see [PCNT API Reference](../modules/pcnt.md).
