# Wake On A GPIO Level With ULP

## Problem Statement

You want the chip to stay in deep sleep and wake only when an RTC-capable GPIO reaches a stable level, without polling from the main CPU.

## Prerequisites

- ESP32 or ESP32-S3
- one RTC-capable GPIO pin
- an external pull-up or pull-down resistor
- ULP FSM enabled in sdkconfig (`CONFIG_ULP_COPROC_ENABLED=y`, `CONFIG_ULP_COPROC_TYPE_FSM=y`)

**Example wiring (wake on button press to GND):**

```
3.3 V --- 10 kOhm --- GPIO4 --- button --- GND
```

Idle level is high through the resistor. Pressing the button pulls the pin low and the ULP wakes the chip after the configured number of stable samples. With the example settings below, keep the button pressed for about 150 ms.

## Minimal Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_ulp_gpio_watch_configure(&(blusys_ulp_gpio_watch_config_t) {
        .pin = 4,
        .wake_on_high = false,
        .period_ms = 50,
        .stable_samples = 3,
    });

    blusys_ulp_start();
    blusys_sleep_enable_ulp_wakeup();
    blusys_sleep_enter_deep();
}
```

After wake, read the retained result at the start of `app_main()`:

```c
blusys_ulp_result_t result;
if (blusys_ulp_get_result(&result) == BLUSYS_OK && result.valid) {
    printf("run_count=%lu last_value=%ld wake_triggered=%d\n",
           (unsigned long) result.run_count,
           (long) result.last_value,
           result.wake_triggered ? 1 : 0);
}
```

## APIs Used

- `blusys_ulp_gpio_watch_configure()` configures one retained GPIO watch job
- `blusys_ulp_start()` loads and starts the ULP FSM program
- `blusys_sleep_enable_ulp_wakeup()` enables ULP as a deep-sleep wake source
- `blusys_sleep_enter_deep()` enters deep sleep
- `blusys_ulp_get_result()` reads the retained result after wake

## Expected Runtime Behavior

- the main CPU enters deep sleep and resets on wake
- the ULP samples the RTC GPIO every `period_ms`
- after `stable_samples` consecutive matches, the ULP wakes the chip
- with `50 ms` and `3` samples, the watched level must stay asserted for about `150 ms`
- `blusys_sleep_get_wakeup_cause()` returns `BLUSYS_SLEEP_WAKEUP_ULP`

## Common Mistakes

- using a non-RTC GPIO pin; `blusys_ulp_gpio_watch_configure()` returns `BLUSYS_ERR_INVALID_ARG`
- relying on the internal pull-up/down; use an external resistor for predictable low-power behavior
- forgetting to enable ULP FSM in sdkconfig; configuration or start returns `BLUSYS_ERR_NOT_SUPPORTED`
- expecting RAM state to survive deep sleep; only RTC slow memory is retained
- expecting a very short pulse to wake the chip; the level must remain asserted for the full debounce window

## Example App

See `examples/ulp_gpio_watch/`.

## API Reference

For full type definitions and function signatures, see [ULP API Reference](../modules/ulp.md).
