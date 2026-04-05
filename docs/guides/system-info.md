# Read System Info

## Problem Statement

You want to print basic runtime information without using raw ESP-IDF APIs directly.

## Prerequisites

- Blusys component available in the ESP-IDF project
- a serial monitor connected to the target board

## Minimal Example

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

## Explanation

- `blusys_system_uptime_us()` reports microseconds since boot
- `blusys_system_reset_reason()` reports the last reset as a compact Blusys enum
- heap helpers report current and minimum free heap in bytes

## Expected Runtime Behavior

The program prints stable diagnostic values such as uptime, reset reason, and heap information.

## Common Mistakes

- passing `NULL` for output pointers
- assuming uptime survives restart or deep sleep
- expecting raw ESP-IDF reset reason values instead of the Blusys enum

## Example App

See `examples/system_info/` for a fuller example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.
