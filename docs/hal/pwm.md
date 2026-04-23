# PWM

Single-output PWM signal with configurable frequency and duty cycle. Duty is expressed in per-mille (‰): 0 = 0%, 500 = 50%, 1000 = 100%.

Each handle owns one LEDC timer and one channel, so at most four handles can be open simultaneously on ESP32-C3 and ESP32-S3.

## Quick Example

```c
blusys_pwm_t *pwm;

blusys_pwm_open(3, 1000, 500, &pwm);
blusys_pwm_start(pwm);
blusys_pwm_set_duty(pwm, 250);
blusys_pwm_close(pwm);
```

## Common Mistakes

- choosing a pin that is not safe for PWM output on the board
- expecting more than four concurrent PWM handles from the current HAL implementation
- passing raw percentages such as `50` when you meant `500` per-mille

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread Safety

- concurrent calls on the same handle are serialized internally
- different handles may be used independently until the timer pool is exhausted
- do not call `blusys_pwm_close()` concurrently with other calls on the same handle

## Limitations

- one dedicated LEDC timer is allocated per handle, so at most four handles can be open simultaneously on ESP32-C3 and ESP32-S3
- duty resolution uses a fixed internal setting; not suitable for extreme frequency/resolution combinations

## Example App

See `examples/reference/hal` (PWM scenario).
