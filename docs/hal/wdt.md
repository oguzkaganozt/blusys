# Watchdog Timer

Task watchdog timer (TWDT): monitor one or more tasks and trigger a panic or log event if any fail to feed the watchdog within the timeout.

The WDT module is global — no open/close handle. One controlling task calls `init`/`deinit`; each monitored task calls `subscribe`/`feed`/`unsubscribe`.

## Quick Example

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_wdt_init(5000, true);   /* 5 s timeout, panic on expiry */
    blusys_wdt_subscribe();

    for (;;) {
        /* critical work here */
        blusys_wdt_feed();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    blusys_wdt_unsubscribe();
    blusys_wdt_deinit();
}
```

## Common Mistakes

- subscribing multiple tasks but only feeding from one — **all** subscribed tasks must feed within the window, or the slowest triggers it
- calling `blusys_wdt_feed()` from an ISR — feed must be called from task context

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

Each task manages its own subscription and feed independently. `blusys_wdt_init()` and `blusys_wdt_deinit()` should be called from a single controlling task.

## ISR Notes

`blusys_wdt_feed()` must not be called from an ISR context.

## Limitations

- monitors the task watchdog only; the interrupt watchdog is managed separately by ESP-IDF
- all subscribed tasks must feed the watchdog within the timeout window; a single slow task will trigger it

## Example App

See `examples/validation/hal_io_lab` (WDT scenario).
