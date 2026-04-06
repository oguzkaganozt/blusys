# Power Management

CPU and APB frequency scaling with automatic light sleep. Reduces power consumption during idle periods while allowing latency-critical code to temporarily lock the CPU at full speed.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [Power Management Basics](../guides/power-mgmt-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes — requires `CONFIG_PM_ENABLE=y` |
| ESP32-C3 | yes — requires `CONFIG_PM_ENABLE=y` |
| ESP32-S3 | yes — requires `CONFIG_PM_ENABLE=y` |

Without `CONFIG_PM_ENABLE=y` in sdkconfig, all functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_POWER_MGMT)` to check at runtime.

## Types

### `blusys_pm_config_t`

```c
typedef struct {
    uint32_t max_freq_mhz;
    uint32_t min_freq_mhz;
    bool     light_sleep_enable;
} blusys_pm_config_t;
```

Power management configuration. `max_freq_mhz` sets the CPU frequency when a `BLUSYS_PM_LOCK_CPU_FREQ_MAX` lock is held or when the system is fully loaded. `min_freq_mhz` sets the CPU frequency when no locks are held and the system is idle. `light_sleep_enable` enables automatic entry into light sleep between FreeRTOS ticks.

Valid frequency values per target:

| Target | Valid frequencies |
|--------|------------------|
| ESP32 | 40 / 80 / 160 / 240 MHz |
| ESP32-C3 | 40 / 80 / 160 MHz |
| ESP32-S3 | 40 / 80 / 160 / 240 MHz |

`min_freq_mhz` must be ≤ `max_freq_mhz`. Setting an invalid combination returns `BLUSYS_ERR_INVALID_ARG` from `blusys_pm_configure`.

### `blusys_pm_lock_type_t`

```c
typedef enum {
    BLUSYS_PM_LOCK_CPU_FREQ_MAX,
    BLUSYS_PM_LOCK_APB_FREQ_MAX,
    BLUSYS_PM_LOCK_NO_LIGHT_SLEEP,
} blusys_pm_lock_type_t;
```

- `BLUSYS_PM_LOCK_CPU_FREQ_MAX` — holds CPU at `max_freq_mhz` while the lock is acquired
- `BLUSYS_PM_LOCK_APB_FREQ_MAX` — holds the APB bus clock at its maximum while the lock is acquired
- `BLUSYS_PM_LOCK_NO_LIGHT_SLEEP` — prevents the system from entering automatic light sleep while the lock is acquired

### `blusys_pm_lock_t`

```c
typedef struct blusys_pm_lock blusys_pm_lock_t;
```

Opaque handle returned by `blusys_pm_lock_create()`.

## Functions

### `blusys_pm_configure`

```c
blusys_err_t blusys_pm_configure(const blusys_pm_config_t *config);
```

Apply a power management configuration. This sets the CPU frequency bounds and enables or disables automatic light sleep. Can be called multiple times to change the configuration at runtime.

**Parameters:**
- `config` — configuration to apply. Must not be NULL.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `config` is NULL or frequencies are invalid for the target, `BLUSYS_ERR_NOT_SUPPORTED` if `CONFIG_PM_ENABLE` is not set.

---

### `blusys_pm_get_configuration`

```c
blusys_err_t blusys_pm_get_configuration(blusys_pm_config_t *out_config);
```

Read the currently active power management configuration. If called before `blusys_pm_configure`, reflects the chip's power-on defaults.

**Parameters:**
- `out_config` — output pointer. Must not be NULL.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `out_config` is NULL, `BLUSYS_ERR_NOT_SUPPORTED` if `CONFIG_PM_ENABLE` is not set.

---

### `blusys_pm_lock_create`

```c
blusys_err_t blusys_pm_lock_create(blusys_pm_lock_type_t type, const char *name,
                                    blusys_pm_lock_t **out_lock);
```

Create a PM lock. Multiple locks of the same or different types can exist concurrently. PM locks are reference-counted: acquiring the same lock handle N times requires N releases before the constraint is lifted.

The `name` pointer is stored by ESP-IDF without copying. It must remain valid for the lifetime of the lock. String literals are safe; stack-allocated strings are not.

**Parameters:**
- `type` — lock type
- `name` — optional debug name. May be NULL.
- `out_lock` — output handle. Must not be NULL.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `out_lock` is NULL or `type` is invalid, `BLUSYS_ERR_NO_MEM` on allocation failure, `BLUSYS_ERR_NOT_SUPPORTED` if `CONFIG_PM_ENABLE` is not set.

---

### `blusys_pm_lock_delete`

```c
blusys_err_t blusys_pm_lock_delete(blusys_pm_lock_t *lock);
```

Delete a PM lock and free its memory. The lock must be fully released (acquire count == 0) before deletion. On failure the handle is left intact.

**Parameters:**
- `lock` — lock handle to delete. Must not be NULL.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `lock` is NULL, `BLUSYS_ERR_INVALID_STATE` if the lock is still acquired.

---

### `blusys_pm_lock_acquire`

```c
blusys_err_t blusys_pm_lock_acquire(blusys_pm_lock_t *lock);
```

Acquire the lock, incrementing its reference count and activating the power constraint.

**Parameters:**
- `lock` — lock handle. Must not be NULL.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `lock` is NULL.

---

### `blusys_pm_lock_release`

```c
blusys_err_t blusys_pm_lock_release(blusys_pm_lock_t *lock);
```

Release the lock, decrementing its reference count. The power constraint is lifted when the reference count reaches zero.

**Parameters:**
- `lock` — lock handle. Must not be NULL.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `lock` is NULL, `BLUSYS_ERR_INVALID_STATE` if the lock is not currently acquired.

## Lifecycle

```
1. blusys_pm_configure()        — set frequency bounds and sleep policy (once at startup)
2. blusys_pm_lock_create()      — create lock(s) as needed
3. blusys_pm_lock_acquire()     — before latency-sensitive or throughput-critical work
4. blusys_pm_lock_release()     — when that work is complete
5. blusys_pm_lock_delete()      — when a lock will no longer be used
```

`blusys_pm_configure()` can be called multiple times to adjust the policy at runtime.

## Thread Safety

- `blusys_pm_configure()` and `blusys_pm_get_configuration()` are safe to call from any task context
- `blusys_pm_lock_acquire()` and `blusys_pm_lock_release()` are safe to call from any task context
- `blusys_pm_lock_create()` and `blusys_pm_lock_delete()` should be called from the task that owns the lock

## ISR Notes

`blusys_pm_lock_acquire()` and `blusys_pm_lock_release()` are ISR-safe. All other functions must not be called from ISR context.

## Limitations

- `CONFIG_PM_ENABLE=y` must be set in the project sdkconfig; without it, all functions return `BLUSYS_ERR_NOT_SUPPORTED`
- frequency values must be valid for the target chip; `blusys_pm_configure` returns `BLUSYS_ERR_INVALID_ARG` for unsupported combinations
- enabling `light_sleep_enable` with an active UART driver may cause missed bytes; call `esp_sleep_enable_uart_wakeup()` to configure UART wakeup if needed
- PM locks are reference-counted; N `acquire` calls require N `release` calls before the constraint is lifted

## Example App

See `examples/power_mgmt_basic/`.
