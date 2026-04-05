# Motor Control PWM

## Purpose

The `mcpwm` module provides a complementary PWM pair suitable for half-bridge and H-bridge motor drivers:

- open a complementary output pair (pin A and pin B) with frequency, duty cycle, and dead-time
- update duty cycle at runtime
- close the handle

## Supported Targets

- ESP32
- ESP32-S3

Not available on ESP32-C3 (no MCPWM hardware).

## Quick Start Example

```c
#include "blusys/mcpwm.h"

void app_main(void)
{
    blusys_mcpwm_t *mcpwm;

    /* 20 kHz, 50% duty, 500 ns dead-time between A and B */
    if (blusys_mcpwm_open(18, 19, 20000, 500, 500, &mcpwm) != BLUSYS_OK) {
        return;
    }

    blusys_mcpwm_set_duty(mcpwm, 700);  /* 70% */
    blusys_mcpwm_close(mcpwm);
}
```

## Lifecycle Model

MCPWM is handle-based:

1. call `blusys_mcpwm_open()` — configures the complementary pair and starts PWM immediately
2. call `blusys_mcpwm_set_duty()` to change duty cycle at any time
3. call `blusys_mcpwm_close()` when finished

## Duty Cycle Units

Duty is expressed in per-mille (‰): 0 = 0%, 500 = 50%, 1000 = 100%.

Pin A follows the configured duty. Pin B is the complement with `dead_time_ns` inserted on both edges.

## Blocking APIs

- `blusys_mcpwm_open()`
- `blusys_mcpwm_close()`
- `blusys_mcpwm_set_duty()`

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_mcpwm_close()` concurrently with other calls using the same handle

## ISR Notes

No ISR-safe calls are defined for the MCPWM module.

## Limitations

- one complementary pair per handle; multiple handles require separate timer and operator resources
- dead-time is symmetric (same nanoseconds applied to rising and falling edges of both outputs)
- available only on targets with MCPWM hardware: ESP32 and ESP32-S3

## Error Behavior

- invalid pins, zero frequency, duty out of range, or null pointer return `BLUSYS_ERR_INVALID_ARG`
- `BLUSYS_ERR_NOT_SUPPORTED` is returned on ESP32-C3
- driver initialization failures return the translated ESP-IDF error

## Example App

See `examples/mcpwm_basic/`.
