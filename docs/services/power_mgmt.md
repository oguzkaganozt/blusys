# Power management

CPU and APB frequency scaling with automatic light sleep. Heavier work can **lock** max CPU frequency; idle intervals drop toward `min_freq_mhz` when light sleep is enabled.

> **Frequencies** are per-chip; the **valid frequencies** table (below) is the source of truth for `*_mhz` values.

## Quick example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_pm_config_t cfg = {
        .max_freq_mhz       = 160,
        .min_freq_mhz       = 40,
        .light_sleep_enable = true,
    };
    blusys_pm_configure(&cfg);

    blusys_pm_lock_t *cpu_lock;
    blusys_pm_lock_create(BLUSYS_PM_LOCK_CPU_FREQ_MAX, "burst", &cpu_lock);

    for (;;) {
        blusys_pm_lock_acquire(cpu_lock);
        /* latency-sensitive or throughput-critical work */
        blusys_pm_lock_release(cpu_lock);

        vTaskDelay(pdMS_TO_TICKS(100));   /* idle — CPU drops to min_freq_mhz */
    }
}
```

Valid frequency values per target:

| Target | Valid frequencies |
|--------|-------------------|
| ESP32 | 40 / 80 / 160 / 240 MHz |
| ESP32-C3 | 40 / 80 / 160 MHz |
| ESP32-S3 | 40 / 80 / 160 / 240 MHz |

`min_freq_mhz` must be ≤ `max_freq_mhz`. Invalid combinations return `BLUSYS_ERR_INVALID_ARG`.

## Common Mistakes

- **`CONFIG_PM_ENABLE` not set in sdkconfig** — all functions return `BLUSYS_ERR_NOT_SUPPORTED`; use `blusys_target_supports(BLUSYS_FEATURE_POWER_MGMT)` at runtime to guard calls
- **missing an `acquire` / `release` pair** — PM locks are reference-counted; N acquires require N releases before the constraint is lifted
- **stack-allocated lock name** — the `name` pointer is stored without copying; use a string literal
- **`light_sleep_enable` with an active UART** — may cause missed bytes unless UART wakeup is configured via `esp_sleep_enable_uart_wakeup()`

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes — requires `CONFIG_PM_ENABLE=y` |
| ESP32-C3 | yes — requires `CONFIG_PM_ENABLE=y` |
| ESP32-S3 | yes — requires `CONFIG_PM_ENABLE=y` |

Without `CONFIG_PM_ENABLE=y` in sdkconfig, all functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_POWER_MGMT)` to check at runtime.

## Thread Safety

- `blusys_pm_configure()` and `blusys_pm_get_configuration()` are safe from any task context
- `blusys_pm_lock_acquire()` and `blusys_pm_lock_release()` are safe from any task context
- `blusys_pm_lock_create()` and `blusys_pm_lock_delete()` should be called from the task that owns the lock

## ISR Notes

- `blusys_pm_lock_acquire()` and `blusys_pm_lock_release()` are ISR-safe
- all other functions must not be called from ISR context

## Limitations

- `CONFIG_PM_ENABLE=y` must be set in the project sdkconfig
- frequency values must be valid for the target chip; invalid combinations return `BLUSYS_ERR_INVALID_ARG`
- PM locks are reference-counted; N `acquire` calls require N `release` calls before the constraint is lifted

## Example App

See `examples/validation/platform_lab/` (`plat_power_mgmt` scenario).
