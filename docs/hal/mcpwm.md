# Motor Control PWM

Complementary PWM pair with configurable dead-time, designed for half-bridge and H-bridge motor drivers.

Pin A follows the duty cycle; pin B is the complement with dead-time inserted on both edges.

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

See `examples/validation/hal_io_lab` (MCPWM scenario).
