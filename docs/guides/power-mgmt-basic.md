# Control CPU Frequency Scaling And Prevent Automatic Sleep

## Problem Statement

You want to reduce power consumption during idle periods by enabling CPU frequency scaling and automatic light sleep. You also need a way for latency-critical code sections to temporarily hold the CPU at full speed or suppress sleep entry.

## Prerequisites

- any supported board (ESP32, ESP32-C3, or ESP32-S3)
- `CONFIG_PM_ENABLE=y` in the project sdkconfig (add to `sdkconfig.defaults`)

## Minimal Example

```c
#include "blusys/power_mgmt.h"

/* Configure frequency scaling and automatic light sleep at startup */
blusys_pm_config_t cfg = {
    .max_freq_mhz      = 80,
    .min_freq_mhz      = 10,
    .light_sleep_enable = true,
};
blusys_pm_configure(&cfg);

/* Pin CPU at max frequency during a latency-sensitive operation */
blusys_pm_lock_t *lock;
blusys_pm_lock_create(BLUSYS_PM_LOCK_CPU_FREQ_MAX, "my_lock", &lock);
blusys_pm_lock_acquire(lock);

/* ... latency-sensitive work ... */

blusys_pm_lock_release(lock);
blusys_pm_lock_delete(lock);
```

## APIs Used

- `blusys_pm_configure()` — sets the CPU frequency range and enables automatic light sleep
- `blusys_pm_get_configuration()` — reads back the active configuration for verification
- `blusys_pm_lock_create()` — creates a lock that can constrain the power state
- `blusys_pm_lock_acquire()` — activates the constraint (CPU stays at max, or sleep is suppressed)
- `blusys_pm_lock_release()` — deactivates the constraint
- `blusys_pm_lock_delete()` — frees the lock when it is no longer needed

## Expected Runtime Behavior

Without any locks held, the CPU scales dynamically between `min_freq_mhz` and `max_freq_mhz` based on the FreeRTOS tick load. When `light_sleep_enable` is true, the system enters light sleep automatically between ticks — RAM is preserved and execution resumes transparently after wakeup.

When a `BLUSYS_PM_LOCK_CPU_FREQ_MAX` lock is acquired, the CPU stays at `max_freq_mhz` until all holders release it. When a `BLUSYS_PM_LOCK_NO_LIGHT_SLEEP` lock is acquired, the system will not enter automatic light sleep until all holders release it.

PM locks are reference-counted. Acquiring the same lock handle N times requires N releases before the constraint is lifted.

## Common Mistakes

**Forgetting `CONFIG_PM_ENABLE=y`**
All functions compile and link successfully but return `BLUSYS_ERR_NOT_SUPPORTED` at runtime. Add `CONFIG_PM_ENABLE=y` to your `sdkconfig.defaults`.

**Setting an unsupported CPU frequency**
Valid frequencies depend on the chip. For ESP32-C3 the maximum is 160 MHz, not 240 MHz. `blusys_pm_configure` returns `BLUSYS_ERR_INVALID_ARG` for invalid values.

**Deleting a lock that is still acquired**
`blusys_pm_lock_delete` returns an error and leaves the handle intact. Always release before deleting.

**Enabling auto light sleep with an active UART driver**
The UART peripheral may miss bytes while the system is in light sleep. Call `esp_sleep_enable_uart_wakeup()` on the relevant UART to configure wakeup on serial activity.

**Passing a stack string as the lock name**
The `name` pointer passed to `blusys_pm_lock_create` is stored by ESP-IDF without copying. A string literal (`"my_lock"`) is safe; a local `char name[]` array is not — it will be freed when the stack frame exits.

## Example App

See `examples/power_mgmt_basic/` for a runnable example that demonstrates configuration, readback, and both lock types.

```bash
./blusys.sh build examples/power_mgmt_basic esp32s3
./blusys.sh run   examples/power_mgmt_basic <port> esp32s3
```

## API Reference

For full type definitions and function signatures, see [Power Management API Reference](../modules/power_mgmt.md).
