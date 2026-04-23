# GPIO

Pin-based digital IO: input, output, pull resistors, and direct ISR callbacks.

GPIO is stateless and pin-based — there is no open/close handle. Configure a pin, use it, reset it when done.

> Function signatures, parameters, and return codes live in `components/blusys/include/blusys/hal/gpio.h` and in the generated API reference.

## Quick Example

```c
#include <stdbool.h>
#include "blusys/blusys.h"

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

**ESP32, ESP32-C3, ESP32-S3** — all supported.

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

## Example Apps

- `examples/reference/hal` (GPIO scenario) — input/output usage
- `examples/validation/hal_io_lab` (GPIO interrupt scenario) — ISR callback usage
