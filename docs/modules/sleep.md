# Sleep Modes

## Purpose

The `sleep` module provides light and deep sleep entry with configurable wakeup sources:

- enable one or more wakeup sources (timer, GPIO, touchpad, UART, EXT0, EXT1)
- enter light sleep (CPU halted, RAM and peripherals retained)
- enter deep sleep (only RTC domain active, RAM lost)
- query the wakeup cause after waking

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include <stdio.h>

#include "blusys/sleep.h"

void app_main(void)
{
    blusys_sleep_enable_timer_wakeup(5000000);   /* wake after 5 s */
    blusys_sleep_enter_light();

    blusys_sleep_wakeup_t cause = blusys_sleep_get_wakeup_cause();
    if (cause == BLUSYS_SLEEP_WAKEUP_TIMER) {
        printf("woke from timer\n");
    }
}
```

## Lifecycle Model

Sleep is stateless (no handle):

1. call one or more `blusys_sleep_enable_*_wakeup()` functions to configure sources
2. call `blusys_sleep_enter_light()` or `blusys_sleep_enter_deep()`
3. after waking from light sleep, continue execution in the same task
4. after waking from deep sleep, execution restarts from `app_main()`
5. call `blusys_sleep_get_wakeup_cause()` to determine what triggered wakeup

## Wakeup Sources

| Source | Function |
|--------|----------|
| Timer | `blusys_sleep_enable_timer_wakeup(us)` |
| GPIO (light sleep only) | via ESP-IDF `gpio_wakeup_enable()` before entering |
| EXT0 (deep sleep, single pin) | via ESP-IDF `esp_sleep_enable_ext0_wakeup()` |
| EXT1 (deep sleep, pin mask) | via ESP-IDF `esp_sleep_enable_ext1_wakeup()` |
| Touchpad | via ESP-IDF `esp_sleep_enable_touchpad_wakeup()` |
| UART | via ESP-IDF `esp_sleep_enable_uart_wakeup()` |

Multiple sources may be enabled simultaneously; the first to fire will wake the device.

## Wakeup Cause Values

- `BLUSYS_SLEEP_WAKEUP_TIMER`
- `BLUSYS_SLEEP_WAKEUP_EXT0`
- `BLUSYS_SLEEP_WAKEUP_EXT1`
- `BLUSYS_SLEEP_WAKEUP_TOUCHPAD`
- `BLUSYS_SLEEP_WAKEUP_GPIO`
- `BLUSYS_SLEEP_WAKEUP_UART`
- `BLUSYS_SLEEP_WAKEUP_UNDEFINED` — cold boot or unknown

## Blocking APIs

- `blusys_sleep_enable_timer_wakeup()`
- `blusys_sleep_enter_light()` — returns after wakeup
- `blusys_sleep_enter_deep()` — does not return; resets on wakeup
- `blusys_sleep_get_wakeup_cause()`

## Thread Safety

Sleep entry is a system-wide event. Only one task should call `blusys_sleep_enter_*()`. Other tasks should be suspended or in a known state before entering sleep.

## ISR Notes

No ISR-safe calls are defined for the sleep module.

## Limitations

- deep sleep causes a full reset; peripheral state, stack, and heap are not preserved
- EXT0 wakeup is available only on ESP32 and ESP32-S3 (not ESP32-C3)
- RTC calendar time survives deep sleep if the RTC slow clock source is stable, but blusys does not expose a calendar API; use the standard C `gettimeofday()` / `settimeofday()` and `esp_sntp` directly for wall-clock time

## Error Behavior

- invalid microsecond values (zero) return `BLUSYS_ERR_INVALID_ARG`
- driver-level failures return the translated ESP-IDF error

## Example App

See `examples/sleep_basic/`.
