# Watchdog Timer

Task watchdog timer (TWDT): monitor one or more tasks and trigger a panic or log event if any fail to feed the watchdog within the timeout.

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

- subscribing multiple tasks but only feeding from one: all subscribed tasks must feed within the window
- calling `blusys_wdt_feed()` from an ISR: feed must be called from task context

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Functions

The WDT module is global (not handle-based). No `open()`/`close()` pattern.

### `blusys_wdt_init`

```c
blusys_err_t blusys_wdt_init(uint32_t timeout_ms, bool panic_on_timeout);
```

Configures and enables the task watchdog. Call once from the controlling task.

**Parameters:**
- `timeout_ms` — watchdog timeout in milliseconds; must be greater than zero
- `panic_on_timeout` — `true` causes a system panic (core dump or reboot) on expiry; `false` logs only

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for zero timeout, translated ESP-IDF error on double-init.

---

### `blusys_wdt_deinit`

```c
blusys_err_t blusys_wdt_deinit(void);
```

Disables the task watchdog. All tasks should unsubscribe before calling this.

---

### `blusys_wdt_subscribe`

```c
blusys_err_t blusys_wdt_subscribe(void);
```

Subscribes the **calling task** to be monitored by the watchdog. Call from each task that should be watched.

**Returns:** `BLUSYS_OK`, translated ESP-IDF error if the task is already subscribed.

---

### `blusys_wdt_unsubscribe`

```c
blusys_err_t blusys_wdt_unsubscribe(void);
```

Unsubscribes the calling task. Call before the task exits.

---

### `blusys_wdt_feed`

```c
blusys_err_t blusys_wdt_feed(void);
```

Resets the watchdog timeout for the calling task. Must be called within `timeout_ms` after the previous feed (or after `subscribe()`).

**Returns:** `BLUSYS_OK`, translated ESP-IDF error if the calling task is not subscribed.

## Lifecycle

1. `blusys_wdt_init()` — enable watchdog (one controlling task)
2. `blusys_wdt_subscribe()` — called by each task that should be monitored
3. `blusys_wdt_feed()` — called regularly within `timeout_ms` from each monitored task
4. `blusys_wdt_unsubscribe()` — called before a monitored task exits
5. `blusys_wdt_deinit()` — disable watchdog (controlling task)

## Thread Safety

Each task manages its own subscription and feed independently. `blusys_wdt_init()` and `blusys_wdt_deinit()` should be called from a single controlling task.

## ISR Notes

`blusys_wdt_feed()` must not be called from an ISR context.

## Limitations

- monitors the task watchdog only; the interrupt watchdog is managed separately by ESP-IDF
- all subscribed tasks must feed the watchdog within the timeout window; a single slow task will trigger it

## Example App

See `examples/validation/hal_io_lab` (WDT scenario).
