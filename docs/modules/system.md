# System

## Purpose

The `system` module exposes a small set of runtime helpers that are common across ESP32-C3 and ESP32-S3:

- restart the chip
- read uptime since boot
- inspect the last reset reason
- inspect current and minimum free heap

## Supported Targets

- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include <inttypes.h>
#include <stdio.h>

#include "blusys/system.h"

void app_main(void)
{
    uint64_t uptime_us;

    if (blusys_system_uptime_us(&uptime_us) == BLUSYS_OK) {
        printf("uptime_us=%" PRIu64 "\n", uptime_us);
    }
}
```

## Lifecycle Model

This module is stateless.
There is no open or close step.

## Blocking APIs

- `blusys_system_restart()`
- `blusys_system_uptime_us()`
- `blusys_system_reset_reason()`
- `blusys_system_free_heap_bytes()`
- `blusys_system_minimum_free_heap_bytes()`

## Async APIs

None in Phase 2.

## Thread Safety

- read helpers are safe to call from multiple tasks
- `blusys_system_restart()` terminates normal program flow and does not return

## ISR Notes

Phase 2 does not define ISR-safety guarantees for this module.

## Limitations

- uptime is based on runtime since boot and resets after restart
- uptime also resets after deep sleep wakeup
- reset reasons are intentionally mapped into a compact Blusys enum

## Error Behavior

- all pointer output arguments return `BLUSYS_ERR_INVALID_ARG` when `NULL`
- successful reads return `BLUSYS_OK`

## Example App

See `examples/system_info/`.
