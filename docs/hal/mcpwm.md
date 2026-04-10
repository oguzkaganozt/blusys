# Motor Control PWM

Complementary PWM pair with configurable dead-time, designed for half-bridge and H-bridge motor drivers.

## Quick Example

```c
blusys_mcpwm_t *mcpwm;

/* 20 kHz, 50% duty, 500 ns dead-time */
blusys_mcpwm_open(18, 19, 20000, 500, 500, &mcpwm);

/* ramp duty from 20% to 80% */
for (uint16_t duty = 200; duty <= 800; duty += 10) {
    blusys_mcpwm_set_duty(mcpwm, duty);
    vTaskDelay(pdMS_TO_TICKS(50));
}

blusys_mcpwm_close(mcpwm);
```

## Common Mistakes

- setting dead-time shorter than the gate driver's minimum blanking time
- using this module on ESP32-C3 (MCPWM is not available; the function returns `BLUSYS_ERR_NOT_SUPPORTED`)
- confusing per-mille (‰) with percent (%) — duty `500` means 50%, not 5%

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | no |
| ESP32-S3 | yes |

On ESP32-C3, all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_MCPWM)` to check at runtime.

## Types

### `blusys_mcpwm_t`

```c
typedef struct blusys_mcpwm blusys_mcpwm_t;
```

Opaque handle returned by `blusys_mcpwm_open()`.

## Functions

### `blusys_mcpwm_open`

```c
blusys_err_t blusys_mcpwm_open(int pin_a,
                               int pin_b,
                               uint32_t freq_hz,
                               uint16_t duty_permille,
                               uint32_t dead_time_ns,
                               blusys_mcpwm_t **out_mcpwm);
```

Configures a complementary PWM pair and starts output immediately. Pin A follows the duty cycle; pin B is the complement with dead-time inserted on both edges.

**Parameters:**
- `pin_a` — GPIO for the primary (non-inverted) output
- `pin_b` — GPIO for the complementary (inverted) output
- `freq_hz` — PWM frequency in Hz
- `duty_permille` — initial duty in per-mille (0–1000; 500 = 50%)
- `dead_time_ns` — dead-time in nanoseconds applied symmetrically to both edges
- `out_mcpwm` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins, zero frequency, duty out of range, or NULL pointer, `BLUSYS_ERR_NOT_SUPPORTED` on ESP32-C3.

---

### `blusys_mcpwm_close`

```c
blusys_err_t blusys_mcpwm_close(blusys_mcpwm_t *mcpwm);
```

Stops the PWM output and releases the handle.

---

### `blusys_mcpwm_set_duty`

```c
blusys_err_t blusys_mcpwm_set_duty(blusys_mcpwm_t *mcpwm, uint16_t duty_permille);
```

Updates the duty cycle at runtime. Changes take effect on the next PWM period.

**Parameters:**
- `duty_permille` — 0 (0%) to 1000 (100%)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if duty is out of range.

## Lifecycle

1. `blusys_mcpwm_open()` — configure pair, output starts immediately
2. `blusys_mcpwm_set_duty()` — update duty at any time
3. `blusys_mcpwm_close()` — stop and release

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_mcpwm_close()` concurrently with other calls on the same handle

## ISR Notes

No ISR-safe calls are defined for the MCPWM module.

## Limitations

- one complementary pair per handle; multiple handles require separate MCPWM timer and operator resources
- dead-time is symmetric (same nanoseconds applied to both edges of both outputs)
- frequency cannot be changed after `open()`; close and reopen with the new frequency

## Example App

See `examples/validation/mcpwm_basic/`.
