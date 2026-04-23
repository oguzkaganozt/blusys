# Sleep Modes

Light and deep sleep with configurable wakeup sources. Light sleep halts the CPU and retains RAM; deep sleep retains only the RTC domain and restarts execution from `app_main()` on wakeup.

## Quick Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_sleep_wakeup_t cause = blusys_sleep_get_wakeup_cause();
    if (cause == BLUSYS_SLEEP_WAKEUP_TIMER) {
        /* woke from previous deep-sleep cycle */
    }

    blusys_sleep_enable_timer_wakeup(5ULL * 1000 * 1000);  /* 5 s */
    blusys_sleep_enter_deep();                             /* does not return */
}
```

## Common Mistakes

- forgetting that deep sleep resets all RAM and peripherals — any state must be stored in NVS or RTC slow memory (`RTC_DATA_ATTR`) before entering
- enabling multiple wakeup sources but only checking one in the handler
- calling a sleep entry function from more than one task simultaneously

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Wakeup Sources

| Source | Enable function | Notes |
|--------|----------------|-------|
| Timer | `blusys_sleep_enable_timer_wakeup(us)` | All targets |
| GPIO (light sleep) | `gpio_wakeup_enable()` (ESP-IDF) | Light sleep only |
| EXT0 (single pin) | `esp_sleep_enable_ext0_wakeup()` (ESP-IDF) | ESP32 and ESP32-S3 only |
| EXT1 (pin mask) | `esp_sleep_enable_ext1_wakeup()` (ESP-IDF) | Deep sleep only |
| Touchpad | `esp_sleep_enable_touchpad_wakeup()` (ESP-IDF) | ESP32 and ESP32-S3 only |
| UART | `esp_sleep_enable_uart_wakeup()` (ESP-IDF) | Light sleep only |
| ULP | `blusys_sleep_enable_ulp_wakeup()` | ESP32 and ESP32-S3 only; requires ULP FSM enabled |

Multiple sources may be enabled simultaneously; the first to fire wakes the device.

## Thread Safety

- sleep entry is a system-wide event; only one task should call `blusys_sleep_enter_*()`
- suspend or coordinate other tasks before entering sleep

## ISR Notes

No ISR-safe calls are defined for the sleep module.

## Limitations

- deep sleep causes a full reset; peripheral state, stack, and heap are not preserved
- EXT0 wakeup is available only on ESP32 and ESP32-S3, not ESP32-C3
- RTC calendar time survives deep sleep if the RTC slow clock source is stable, but Blusys does not expose a calendar API; use `gettimeofday()` / `settimeofday()` directly

## Example App

See `examples/validation/hal_io_lab` (sleep scenario).
