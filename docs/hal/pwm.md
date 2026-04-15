# PWM

Single-output PWM signal with configurable frequency and duty cycle. Duty is expressed in per-mille (‰): 0 = 0%, 500 = 50%, 1000 = 100%.

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

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_pwm_t`

```c
typedef struct blusys_pwm blusys_pwm_t;
```

Opaque handle returned by `blusys_pwm_open()`. Each handle owns one LEDC timer and one channel.

## Functions

### `blusys_pwm_open`

```c
blusys_err_t blusys_pwm_open(int pin,
                             uint32_t freq_hz,
                             uint16_t duty_permille,
                             blusys_pwm_t **out_pwm);
```

Allocates a LEDC timer and channel for the given pin. The output is not active until `blusys_pwm_start()` is called.

**Parameters:**
- `pin` — GPIO number
- `freq_hz` — PWM frequency in Hz
- `duty_permille` — initial duty cycle in per-mille (0–1000)
- `out_pwm` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pin, frequency, or duty values, `BLUSYS_ERR_BUSY` if the timer pool is exhausted (at most four handles open at once on C3/S3).

---

### `blusys_pwm_close`

```c
blusys_err_t blusys_pwm_close(blusys_pwm_t *pwm);
```

Stops the output, releases the timer and channel, and frees the handle.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `pwm` is NULL.

---

### `blusys_pwm_set_frequency`

```c
blusys_err_t blusys_pwm_set_frequency(blusys_pwm_t *pwm, uint32_t freq_hz);
```

Changes the PWM frequency while the output may be running.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for zero or out-of-range frequency.

---

### `blusys_pwm_set_duty`

```c
blusys_err_t blusys_pwm_set_duty(blusys_pwm_t *pwm, uint16_t duty_permille);
```

Changes the duty cycle. The new duty takes effect on the next PWM period.

**Parameters:**
- `duty_permille` — 0 (0%) to 1000 (100%)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if duty is out of range.

---

### `blusys_pwm_start`

```c
blusys_err_t blusys_pwm_start(blusys_pwm_t *pwm);
```

Enables PWM output on the pin.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `pwm` is NULL.

---

### `blusys_pwm_stop`

```c
blusys_err_t blusys_pwm_stop(blusys_pwm_t *pwm);
```

Disables PWM output without releasing the handle. The pin goes idle. Call `blusys_pwm_start()` to resume.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `pwm` is NULL.

## Lifecycle

1. `blusys_pwm_open()` — allocate timer and channel
2. `blusys_pwm_start()` — enable output
3. `blusys_pwm_set_duty()` / `blusys_pwm_set_frequency()` — adjust as needed
4. `blusys_pwm_stop()` — pause output (optional)
5. `blusys_pwm_close()` — release

## Thread Safety

- concurrent calls on the same handle are serialized internally
- different handles may be used independently until the timer pool is exhausted
- do not call `blusys_pwm_close()` concurrently with other calls on the same handle

## Limitations

- one dedicated LEDC timer is allocated per handle, so at most four handles can be open simultaneously on ESP32-C3 and ESP32-S3
- duty resolution uses a fixed internal setting; not suitable for extreme frequency/resolution combinations

## Example App

See `examples/reference/hal` (PWM scenario).
