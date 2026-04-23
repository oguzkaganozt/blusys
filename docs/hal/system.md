# System

Runtime helpers common across all supported targets: uptime, reset reason, heap statistics, and soft restart.

> **API reference:** `components/blusys/include/blusys/hal/system.h` and the generated API reference.

## Quick Example

```c
#include <inttypes.h>
#include <stdio.h>

#include "blusys/blusys.h"

void app_main(void)
{
    uint64_t uptime_us;

    if (blusys_system_uptime_us(&uptime_us) == BLUSYS_OK) {
        printf("uptime_us=%" PRIu64 "\n", uptime_us);
    }
}
```

## Common Mistakes

- passing `NULL` for output pointers
- assuming uptime survives restart or deep sleep
- expecting raw ESP-IDF reset reason values instead of the Blusys enum

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread Safety

- all read functions are safe to call from multiple tasks concurrently
- `blusys_system_restart()` terminates normal program flow and does not return

## ISR Notes

No ISR-safe calls are defined for the system module.

## Limitations

- uptime resets after a software restart or deep sleep wakeup
- reset reasons are mapped into a compact Blusys enum; unknown ESP-IDF reasons map to `BLUSYS_RESET_REASON_UNKNOWN`

## Example App

See `examples/validation/hal_io_lab/` (menuconfig: system_info scenario).
