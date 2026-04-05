# Monitor A Task With The Watchdog Timer

## Problem Statement

You want the system to detect and recover from a stuck or hung task by enabling the task watchdog timer.

## Prerequisites

- any supported board
- a task that runs a periodic loop and must be verified to make progress

## Minimal Example

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/wdt.h"

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

## APIs Used

- `blusys_wdt_init()` configures the task watchdog timeout in milliseconds and whether to panic or only log on expiry
- `blusys_wdt_subscribe()` registers the calling task with the watchdog
- `blusys_wdt_feed()` resets the countdown for the calling task
- `blusys_wdt_unsubscribe()` removes the calling task from watchdog supervision
- `blusys_wdt_deinit()` disables and frees the watchdog

## Expected Runtime Behavior

- if `blusys_wdt_feed()` is not called within the timeout window, the watchdog fires
- with `panic_on_timeout = true`, the system prints a backtrace and resets (or halts for JTAG)
- with `panic_on_timeout = false`, a warning is logged but the system continues

## Common Mistakes

- subscribing multiple tasks but only feeding from one: all subscribed tasks must feed within the window
- calling `blusys_wdt_feed()` from an ISR: feed must be called from task context

## Example App

See `examples/wdt_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.
