# GPIO

Pin-based digital IO: input, output, pull resistors, and direct ISR callbacks.

## Quick Example

```c
#include <stdbool.h>

#include "blusys/gpio.h"

void app_main(void)
{
    bool level;

    blusys_gpio_reset(3);
    blusys_gpio_set_output(3);
    blusys_gpio_write(3, true);

    blusys_gpio_reset(2);
    blusys_gpio_set_input(2);
    blusys_gpio_set_pull_mode(2, BLUSYS_GPIO_PULL_UP);
    blusys_gpio_read(2, &level);
}
```

## Common Mistakes

- choosing pins that are valid on the SoC but reserved on the specific board
- writing to a pin that was not configured for output
- reading a floating input without setting an external or internal pull resistor

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_gpio_pull_t`

```c
typedef enum {
    BLUSYS_GPIO_PULL_NONE = 0,
    BLUSYS_GPIO_PULL_UP,
    BLUSYS_GPIO_PULL_DOWN,
    BLUSYS_GPIO_PULL_UP_DOWN,
} blusys_gpio_pull_t;
```

### `blusys_gpio_interrupt_mode_t`

```c
typedef enum {
    BLUSYS_GPIO_INTERRUPT_DISABLED = 0,
    BLUSYS_GPIO_INTERRUPT_RISING,
    BLUSYS_GPIO_INTERRUPT_FALLING,
    BLUSYS_GPIO_INTERRUPT_ANY_EDGE,
    BLUSYS_GPIO_INTERRUPT_LOW_LEVEL,
    BLUSYS_GPIO_INTERRUPT_HIGH_LEVEL,
} blusys_gpio_interrupt_mode_t;
```

### `blusys_gpio_callback_t`

```c
typedef bool (*blusys_gpio_callback_t)(int pin, void *user_ctx);
```

Callback invoked directly in GPIO ISR context. Return `true` to request a FreeRTOS yield after waking a higher-priority task.

## Functions

### `blusys_gpio_reset`

```c
blusys_err_t blusys_gpio_reset(int pin);
```

Resets a pin to its default hardware state (input, no pull, no interrupt, no callback).

**Parameters:**
- `pin` — GPIO number

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins.

---

### `blusys_gpio_set_input`

```c
blusys_err_t blusys_gpio_set_input(int pin);
```

Configures the pin as a digital input.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins.

---

### `blusys_gpio_set_output`

```c
blusys_err_t blusys_gpio_set_output(int pin);
```

Configures the pin as a digital output.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins.

---

### `blusys_gpio_set_pull_mode`

```c
blusys_err_t blusys_gpio_set_pull_mode(int pin, blusys_gpio_pull_t pull);
```

Sets the internal pull resistor configuration for the pin.

**Parameters:**
- `pin` — GPIO number
- `pull` — pull mode

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins or pull values.

---

### `blusys_gpio_read`

```c
blusys_err_t blusys_gpio_read(int pin, bool *out_level);
```

Reads the current digital level of the pin. The pin should be configured as input for meaningful results.

**Parameters:**
- `pin` — GPIO number
- `out_level` — output pointer: `true` for high, `false` for low

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins or NULL pointer.

---

### `blusys_gpio_write`

```c
blusys_err_t blusys_gpio_write(int pin, bool level);
```

Sets the output level of the pin. The pin should be configured as output.

**Parameters:**
- `pin` — GPIO number
- `level` — `true` for high, `false` for low

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins.

---

### `blusys_gpio_set_interrupt`

```c
blusys_err_t blusys_gpio_set_interrupt(int pin, blusys_gpio_interrupt_mode_t mode);
```

Configures the interrupt trigger for a pin. Set to `BLUSYS_GPIO_INTERRUPT_DISABLED` to disable.

**Parameters:**
- `pin` — GPIO number
- `mode` — trigger mode

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.

---

### `blusys_gpio_set_callback`

```c
blusys_err_t blusys_gpio_set_callback(int pin, blusys_gpio_callback_t callback, void *user_ctx);
```

Registers a callback that runs directly in GPIO ISR context when the interrupt fires. Pass `NULL` for `callback` to clear the registered callback.

**Parameters:**
- `pin` — GPIO number
- `callback` — ISR-context callback, or NULL to clear
- `user_ctx` — passed back to the callback unchanged

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins, `BLUSYS_ERR_INVALID_STATE` if a non-disabled interrupt mode is not configured first.

## Lifecycle

GPIO is stateless and pin-based — no open/close handle. Configure a pin, use it, reset it when done.

## Thread Safety

- different pins may be controlled independently from different tasks
- Blusys does not coordinate concurrent access to the same pin
- read-modify-write patterns such as toggle are intentionally not part of this API
- callback registration and deregistration are safe for one pin at a time, but an in-flight callback may finish before deregistration completes

## ISR Notes

- callbacks run in ISR context
- the callback must not block
- the callback must not call normal blocking Blusys APIs
- if the callback wakes a higher-priority task, return `true`
- if `CONFIG_GPIO_CTRL_FUNC_IN_IRAM` is enabled, callback code must follow the ESP-IDF IRAM-safe callback rules

## Limitations

- pin validation checks SoC-valid GPIO numbers, not board-safe wiring choices
- some boards reserve pins for flash, PSRAM, USB, JTAG, or onboard peripherals
- one callback can be registered per pin
- interrupt disable leaves the registered callback in place until explicitly cleared or the pin is reset
- interrupt delivery is only armed when both a non-disabled interrupt mode and a registered callback are present

## Example App

See `examples/reference/gpio_basic/` for input/output usage.
See `examples/validation/gpio_interrupt/` for ISR callback usage.
