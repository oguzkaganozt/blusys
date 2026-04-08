# ULP

Ultra Low Power coprocessor helpers for deep-sleep wakeup tasks. V1 exposes one built-in job: sampling an RTC-capable GPIO at a fixed interval and waking the main CPU when the watched level is stable.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [Wake On A GPIO Level With ULP](../guides/ulp-gpio-watch.md).

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

## Types

### `blusys_ulp_job_t`

```c
typedef enum {
    BLUSYS_ULP_JOB_NONE = 0,
    BLUSYS_ULP_JOB_GPIO_WATCH,
} blusys_ulp_job_t;
```

### `blusys_ulp_gpio_watch_config_t`

```c
typedef struct {
    int      pin;
    bool     wake_on_high;
    uint32_t period_ms;
    uint8_t  stable_samples;
} blusys_ulp_gpio_watch_config_t;
```

| Field | Description |
|-------|-------------|
| `pin` | RTC-capable GPIO number to sample |
| `wake_on_high` | `true` to wake on logic high, `false` to wake on logic low |
| `period_ms` | sample period in milliseconds |
| `stable_samples` | number of consecutive matching samples required |

### `blusys_ulp_result_t`

```c
typedef struct {
    bool             valid;
    blusys_ulp_job_t job;
    uint32_t         run_count;
    int32_t          last_value;
    bool             wake_triggered;
} blusys_ulp_result_t;
```

`last_value` is `0` or `1` after at least one sample, and `-1` if the current retained result does not yet contain a sampled GPIO value.

`run_count` is stored in a 16-bit ULP shared-memory word and wraps after `65535` samples.

## Functions

### `blusys_ulp_gpio_watch_configure`

```c
blusys_err_t blusys_ulp_gpio_watch_configure(const blusys_ulp_gpio_watch_config_t *config);
```

Configures the built-in GPIO watch job, prepares the selected pin as an RTC input, and initializes the retained result block.

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`, translated backend error on load/setup failure, or `BLUSYS_ERR_NOT_SUPPORTED`.

---

### `blusys_ulp_start`

```c
blusys_err_t blusys_ulp_start(void);
```

Starts the configured ULP job. Call `blusys_sleep_enable_ulp_wakeup()` and then `blusys_sleep_enter_deep()` to let the ULP wake the chip.

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if no job is configured, translated ESP-IDF error on startup failure, or `BLUSYS_ERR_NOT_SUPPORTED`.

---

### `blusys_ulp_stop`

```c
blusys_err_t blusys_ulp_stop(void);
```

Stops the ULP timer. Does not clear the retained result.

`blusys_ulp_stop()` also releases the watched GPIO from RTC hold/input mode. Call `blusys_ulp_gpio_watch_configure()` again before starting a new watch cycle.

---

### `blusys_ulp_clear_result`

```c
blusys_err_t blusys_ulp_clear_result(void);
```

Clears the retained run count, last sampled value, and wake flag for the current job.

Use this before re-entering deep sleep if you want to arm a fresh debounce cycle without reconfiguring the job.

---

### `blusys_ulp_get_result`

```c
blusys_err_t blusys_ulp_get_result(blusys_ulp_result_t *out_result);
```

Reads the retained result written by the ULP program. Safe to call at the start of `app_main()` after deep-sleep wake.

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, or `BLUSYS_ERR_NOT_SUPPORTED`.

## Lifecycle

1. Call `blusys_ulp_gpio_watch_configure()`.
2. Call `blusys_ulp_start()`.
3. Enable wakeup via `blusys_sleep_enable_ulp_wakeup()`.
4. Enter deep sleep with `blusys_sleep_enter_deep()`.
5. After wake, call `blusys_ulp_get_result()` and `blusys_sleep_get_wakeup_cause()`.

## Thread Safety

All public functions are serialized by one global internal lock. This module is process-wide and supports one active job at a time.

## ISR Notes

No ISR-safe calls are defined for this module.

## Limitations

- one built-in job only: GPIO watch
- RTC-capable GPIO pins only
- deep-sleep wake use case only; no generic ULP program loading API is exposed
- retained counters are based on the ULP shared memory words and are intended for lightweight wake diagnostics, not long-term persistent statistics

## Example App

See `examples/ulp_gpio_watch/`.
