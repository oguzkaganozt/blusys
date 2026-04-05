# Basic PWM Output

## Problem Statement

You want to drive a PWM-capable GPIO with a simple repeating duty-cycle pattern through the Blusys API.

## Prerequisites

- a supported board
- one PWM-safe output pin connected to the load you want to drive
- the example pin reviewed in `idf.py menuconfig` if your board uses a different safe output pin

## Minimal Example

```c
blusys_pwm_t *pwm;

blusys_pwm_open(3, 1000, 500, &pwm);
blusys_pwm_start(pwm);
blusys_pwm_set_duty(pwm, 250);
blusys_pwm_close(pwm);
```

## APIs Used

- `blusys_pwm_open()` allocates one PWM handle for a pin, frequency, and initial duty
- `blusys_pwm_start()` enables output using the current duty setting
- `blusys_pwm_set_duty()` changes the duty cycle using per-mille values
- `blusys_pwm_close()` disables the output and releases the backend resources

## Expected Runtime Behavior

- the example prints the configured target and GPIO pin
- PWM output starts at 1 kHz and cycles through a few duty levels once per second
- the pin remains low after the handle is closed

## Common Mistakes

- choosing a pin that is not safe for PWM output on the board
- expecting more than four concurrent PWM handles from the current Phase 4 implementation
- passing raw percentages such as `50` when you meant `500` per-mille

## Example App

See `examples/pwm_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.
