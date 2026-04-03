# PWM

## Purpose

The `pwm` module provides a blocking handle-based API for the common single-output PWM path:

- open one PWM output on a GPIO pin
- set frequency
- set duty cycle in per-mille
- start and stop output
- close the PWM handle

Phase 4 keeps the API intentionally smaller than ESP-IDF LEDC and allocates backend timer/channel resources internally.

## Supported Targets

- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include "blusys/pwm.h"

void app_main(void)
{
    blusys_pwm_t *pwm;

    if (blusys_pwm_open(3, 1000, 500, &pwm) != BLUSYS_OK) {
        return;
    }

    blusys_pwm_start(pwm);
    blusys_pwm_set_duty(pwm, 250);
    blusys_pwm_close(pwm);
}
```

## Lifecycle Model

PWM is handle-based:

1. call `blusys_pwm_open()`
2. call `blusys_pwm_start()` when you want output enabled
3. use `blusys_pwm_set_frequency()` and `blusys_pwm_set_duty()` as needed
4. call `blusys_pwm_stop()` when you want the output disabled without freeing the handle
5. call `blusys_pwm_close()` when finished

## Blocking APIs

- `blusys_pwm_open()`
- `blusys_pwm_close()`
- `blusys_pwm_set_frequency()`
- `blusys_pwm_set_duty()`
- `blusys_pwm_start()`
- `blusys_pwm_stop()`

## Async APIs

None in Phase 4.

## Thread Safety

- concurrent calls using the same handle are serialized internally
- different PWM handles may be used independently until the current internal timer allocation pool is exhausted
- do not call `blusys_pwm_close()` concurrently with other calls using the same handle

## ISR Notes

Phase 4 does not define ISR-safe PWM calls.

## Limitations

- duty is expressed as `0` to `1000` per-mille instead of exposing raw LEDC duty counts
- the current implementation uses one dedicated LEDC timer per Blusys PWM handle, so at most four PWM handles can be open at once on the common C3/S3 subset
- the current implementation uses a fixed internal duty resolution and is aimed at normal low-frequency PWM use rather than extreme frequency/resolution combinations

## Error Behavior

- invalid pins, frequency values, duty values, or pointers return `BLUSYS_ERR_INVALID_ARG`
- opening more PWM handles than the current common timer pool supports returns `BLUSYS_ERR_BUSY`
- backend timer or channel allocation failures are translated into `blusys_err_t`

## Example App

See `examples/pwm_basic/`.
