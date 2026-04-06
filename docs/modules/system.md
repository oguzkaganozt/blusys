# System

Runtime helpers common across all supported targets: uptime, reset reason, heap statistics, and soft restart.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [System Info](../guides/system-info.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_reset_reason_t`

```c
typedef enum {
    BLUSYS_RESET_REASON_UNKNOWN = 0,
    BLUSYS_RESET_REASON_POWER_ON,
    BLUSYS_RESET_REASON_EXTERNAL,
    BLUSYS_RESET_REASON_SOFTWARE,
    BLUSYS_RESET_REASON_PANIC,
    BLUSYS_RESET_REASON_INTERRUPT_WATCHDOG,
    BLUSYS_RESET_REASON_TASK_WATCHDOG,
    BLUSYS_RESET_REASON_WATCHDOG,
    BLUSYS_RESET_REASON_DEEP_SLEEP,
    BLUSYS_RESET_REASON_BROWNOUT,
    BLUSYS_RESET_REASON_SDIO,
    BLUSYS_RESET_REASON_USB,
    BLUSYS_RESET_REASON_JTAG,
    BLUSYS_RESET_REASON_EFUSE,
    BLUSYS_RESET_REASON_POWER_GLITCH,
    BLUSYS_RESET_REASON_CPU_LOCKUP,
} blusys_reset_reason_t;
```

## Functions

### `blusys_system_restart`

```c
void blusys_system_restart(void);
```

Triggers a software reset. Does not return.

---

### `blusys_system_uptime_us`

```c
blusys_err_t blusys_system_uptime_us(uint64_t *out_us);
```

Returns microseconds elapsed since boot.

**Parameters:**
- `out_us` — output pointer for the uptime value

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if `out_us` is NULL.

---

### `blusys_system_reset_reason`

```c
blusys_err_t blusys_system_reset_reason(blusys_reset_reason_t *out_reason);
```

Returns the reason for the last reset.

**Parameters:**
- `out_reason` — output pointer for the reset reason enum value

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if `out_reason` is NULL.

---

### `blusys_system_free_heap_bytes`

```c
blusys_err_t blusys_system_free_heap_bytes(size_t *out_bytes);
```

Returns the current free heap size in bytes.

**Parameters:**
- `out_bytes` — output pointer for the heap size

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if `out_bytes` is NULL.

---

### `blusys_system_minimum_free_heap_bytes`

```c
blusys_err_t blusys_system_minimum_free_heap_bytes(size_t *out_bytes);
```

Returns the minimum free heap size recorded since boot. Useful for detecting memory pressure over time.

**Parameters:**
- `out_bytes` — output pointer for the watermark value

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if `out_bytes` is NULL.

## Lifecycle

This module is stateless — no open or close step. Call any function at any time after `app_main()` starts.

## Thread Safety

All read functions are safe to call from multiple tasks concurrently. `blusys_system_restart()` terminates normal program flow and does not return.

## Limitations

- uptime resets after a software restart or deep sleep wakeup
- reset reasons are mapped into a compact Blusys enum; unknown ESP-IDF reasons map to `BLUSYS_RESET_REASON_UNKNOWN`

## Example App

See `examples/system_info/`.
