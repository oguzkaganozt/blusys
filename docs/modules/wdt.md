# Watchdog Timer

## Purpose

The `wdt` module exposes the ESP-IDF task watchdog timer (TWDT):

- initialize the watchdog with a timeout and optional panic behavior
- subscribe the current task to be monitored
- feed the watchdog to reset the timeout
- unsubscribe and deinitialize when done

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/wdt.h"

void app_main(void)
{
    blusys_wdt_init(5000, true);   /* 5 s timeout, panic on expiry */
    blusys_wdt_subscribe();

    for (;;) {
        /* do work */
        blusys_wdt_feed();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

## Lifecycle Model

WDT is global (not handle-based):

1. call `blusys_wdt_init()` — configures the TWDT with a timeout
2. call `blusys_wdt_subscribe()` in each task that should be monitored
3. call `blusys_wdt_feed()` regularly to prevent expiry
4. call `blusys_wdt_unsubscribe()` before a task exits
5. call `blusys_wdt_deinit()` to stop the watchdog

## Blocking APIs

- `blusys_wdt_init()`
- `blusys_wdt_deinit()`
- `blusys_wdt_subscribe()`
- `blusys_wdt_unsubscribe()`
- `blusys_wdt_feed()`

## Thread Safety

Each task manages its own subscription and feed independently. `blusys_wdt_init()` and `blusys_wdt_deinit()` should be called from a single controlling task.

## ISR Notes

`blusys_wdt_feed()` must not be called from an ISR context.

## Limitations

- monitors the task watchdog only; the interrupt watchdog is managed separately by ESP-IDF
- `panic_on_timeout = true` causes a system panic (core dump or reboot) on expiry; `false` logs only
- all subscribed tasks must feed the watchdog within the timeout window; a single slow task will trigger it

## Error Behavior

- zero timeout or double-init return `BLUSYS_ERR_INVALID_ARG` or the translated ESP-IDF error
- calling `blusys_wdt_feed()` without subscribing returns an error from the underlying TWDT

## Example App

See `examples/wdt_basic/`.
