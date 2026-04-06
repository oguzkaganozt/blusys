# Drive A Half-Bridge With Complementary PWM

## Problem Statement

You want to drive a half-bridge motor driver (e.g. DRV8301, IR2110) that requires two non-overlapping PWM signals with dead-time between them.

## Prerequisites

- a supported board (ESP32 or ESP32-S3)
- two GPIO pins connected to the high-side and low-side gate driver inputs
- the gate driver's minimum dead-time requirement

## Minimal Example

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

## APIs Used

- `blusys_mcpwm_open()` configures pin A (high side), pin B (low side), frequency in Hz, duty in per-mille (‰), and dead-time in nanoseconds
- `blusys_mcpwm_set_duty()` updates duty at runtime; pin B is always the complement of pin A with dead-time applied to both edges
- `blusys_mcpwm_close()` releases the handle and stops both outputs

## Duty Cycle Units

Duty is in per-mille: 0 = 0%, 500 = 50%, 1000 = 100%.

## Expected Runtime Behavior

- pin A and pin B carry complementary waveforms
- neither pin is high simultaneously; dead-time ensures a gap before any transition
- `set_duty()` takes effect on the next PWM period

## Common Mistakes

- setting dead-time shorter than the gate driver's minimum blanking time
- using this module on ESP32-C3 (MCPWM is not available; the function returns `BLUSYS_ERR_NOT_SUPPORTED`)
- confusing per-mille (‰) with percent (%) — duty `500` means 50%, not 5%

## Example App

See `examples/mcpwm_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.


## API Reference

For full type definitions and function signatures, see [MCPWM API Reference](../modules/mcpwm.md).
