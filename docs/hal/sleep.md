# Sleep Modes

Light and deep sleep with configurable wakeup sources. Light sleep halts the CPU and retains RAM; deep sleep retains only the RTC domain.

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

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_sleep_wakeup_t`

```c
typedef enum {
    BLUSYS_SLEEP_WAKEUP_UNDEFINED = 0, /* cold boot or unknown reason */
    BLUSYS_SLEEP_WAKEUP_TIMER,
    BLUSYS_SLEEP_WAKEUP_EXT0,
    BLUSYS_SLEEP_WAKEUP_EXT1,
    BLUSYS_SLEEP_WAKEUP_TOUCHPAD,
    BLUSYS_SLEEP_WAKEUP_GPIO,
    BLUSYS_SLEEP_WAKEUP_UART,
    BLUSYS_SLEEP_WAKEUP_ULP,
} blusys_sleep_wakeup_t;
```

## Functions

### `blusys_sleep_enable_timer_wakeup`

```c
blusys_err_t blusys_sleep_enable_timer_wakeup(uint64_t us);
```

Configures the RTC timer as a wakeup source. The device wakes after `us` microseconds.

**Parameters:**
- `us` — sleep duration in microseconds; must be greater than zero

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for zero duration, translated ESP-IDF error on failure.

---

### `blusys_sleep_enable_ulp_wakeup`

```c
blusys_err_t blusys_sleep_enable_ulp_wakeup(void);
```

Enables the ULP coprocessor as a deep-sleep wakeup source. Use this together with `blusys_ulp_start()`.

Returns translated ESP-IDF error codes if ULP is unavailable or not enabled in sdkconfig.

---

### `blusys_sleep_enter_light`

```c
blusys_err_t blusys_sleep_enter_light(void);
```

Enters light sleep. The CPU halts and RAM/peripheral state is preserved. Returns after the first wakeup source fires. Execution continues in the calling task from the next instruction.

---

### `blusys_sleep_enter_deep`

```c
blusys_err_t blusys_sleep_enter_deep(void);
```

Enters deep sleep. Does not return — the device resets on wakeup and execution restarts from `app_main()`. Only the RTC slow memory and RTC peripherals remain active.

---

### `blusys_sleep_get_wakeup_cause`

```c
blusys_sleep_wakeup_t blusys_sleep_get_wakeup_cause(void);
```

Returns the wakeup cause after waking from light or deep sleep. Call at the start of `app_main()` or after `blusys_sleep_enter_light()` returns.

Returns `BLUSYS_SLEEP_WAKEUP_UNDEFINED` after a cold power-on or unknown reset.

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

## Lifecycle

This module is stateless — no handle.

1. Call one or more wakeup-source enable functions
2. Call `blusys_sleep_enter_light()` or `blusys_sleep_enter_deep()`
3. After waking, call `blusys_sleep_get_wakeup_cause()` to determine the trigger

## Thread Safety

Sleep entry is a system-wide event. Only one task should call `blusys_sleep_enter_*()`. Suspend or coordinate other tasks before entering sleep.

## ISR Notes

No ISR-safe calls are defined for the sleep module.

## Limitations

- deep sleep causes a full reset; peripheral state, stack, and heap are not preserved
- EXT0 wakeup is available only on ESP32 and ESP32-S3, not ESP32-C3
- RTC calendar time survives deep sleep if the RTC slow clock source is stable, but Blusys does not expose a calendar API; use `gettimeofday()` / `settimeofday()` directly

## Example App

See `examples/validation/hal_io_lab` (sleep scenario).
