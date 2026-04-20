# ULP

Ultra Low Power coprocessor helpers for deep-sleep wakeup tasks. V1 exposes one built-in job: sampling an RTC-capable GPIO at a fixed interval and waking the main CPU when the watched level is stable.

## Quick Example

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

`last_value` is `0` or `1` after at least one sample, and `-1` until the ULP has captured its first GPIO sample. `run_count` is stored in a 16-bit ULP shared-memory word and wraps after `65535` samples.

## Common Mistakes

- using a non-RTC GPIO pin; `blusys_ulp_gpio_watch_configure()` returns `BLUSYS_ERR_INVALID_ARG`
- relying on the internal pull-up/down — use an external resistor for predictable low-power behavior
- forgetting to enable ULP FSM in sdkconfig; configuration or start returns `BLUSYS_ERR_NOT_SUPPORTED`
- expecting RAM state to survive deep sleep — only RTC slow memory is retained
- expecting a very short pulse to wake the chip — the level must remain asserted for the full debounce window

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | no |
| ESP32-S3 | yes |

This module requires the ESP-IDF ULP FSM coprocessor support to be enabled in sdkconfig:

- `CONFIG_ULP_COPROC_ENABLED=y`
- `CONFIG_ULP_COPROC_TYPE_FSM=y`
- `CONFIG_ULP_COPROC_RESERVE_MEM` large enough for the embedded ULP program

On unsupported targets, or when ULP FSM is not enabled, all functions return `BLUSYS_ERR_NOT_SUPPORTED`.

## Thread Safety

- all public functions are serialized by one global internal lock
- process-wide; supports one active job at a time

## ISR Notes

No ISR-safe calls are defined for the ULP module.

## Limitations

- one built-in job only: GPIO watch
- RTC-capable GPIO pins only
- deep-sleep wake use case only; no generic ULP program loading API is exposed
- retained counters come from ULP shared memory words and are intended for lightweight wake diagnostics, not long-term persistent statistics
- `blusys_ulp_stop()` releases the watched GPIO from RTC hold/input mode; reconfigure before the next watch cycle

## Example App

See `examples/validation/hal_io_lab` (ULP GPIO scenario).
