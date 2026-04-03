# GPIO

## Purpose

The `gpio` module provides a direct pin-based API for the most common digital IO tasks:

- reset a pin to its default state
- set input or output mode
- configure pull resistors
- read a pin level
- write a pin level
- configure one interrupt trigger per pin
- register one direct ISR callback per pin

## Supported Targets

- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include "blusys/gpio.h"

void app_main(void)
{
    blusys_gpio_reset(3);
    blusys_gpio_set_output(3);
    blusys_gpio_write(3, true);
}
```

## Lifecycle Model

GPIO is pin-based and stateless.
There is no handle model in Phase 2.

## Blocking APIs

- `blusys_gpio_reset()`
- `blusys_gpio_set_input()`
- `blusys_gpio_set_output()`
- `blusys_gpio_set_pull_mode()`
- `blusys_gpio_read()`
- `blusys_gpio_write()`

## Async APIs

- `blusys_gpio_set_interrupt()` configures one interrupt trigger for a pin
- `blusys_gpio_set_callback()` registers one callback that runs directly in GPIO ISR context

The callback return value is used to request a yield when a higher-priority task is woken from ISR context.

## Thread Safety

- different pins may be controlled independently from different tasks
- Blusys does not coordinate concurrent access to the same pin
- read-modify-write patterns such as toggle are intentionally not part of this first API
- callback registration and deregistration are safe for one pin at a time, but an in-flight callback may finish before deregistration completes

## ISR Notes

- GPIO callbacks run in ISR context
- the callback must not block
- the callback must not call normal blocking Blusys APIs
- if the callback wakes a higher-priority task, return `true`
- if `CONFIG_GPIO_CTRL_FUNC_IN_IRAM` is enabled, callback code must follow the ESP-IDF IRAM-safe callback rules when it touches GPIO control functions

## Limitations

- pin validation checks SoC-valid GPIO numbers, not board-safe wiring choices
- some boards reserve pins for flash, PSRAM, USB, JTAG, or onboard peripherals
- `blusys_gpio_read()` expects the pin to have been configured for input when meaningful input sampling is required
- one callback can be registered per pin
- interrupt disable leaves the registered callback in place until it is explicitly cleared or the pin is reset
- interrupt delivery is only armed while both a non-disabled interrupt mode and a registered callback are present

## Error Behavior

- invalid pins return `BLUSYS_ERR_INVALID_ARG`
- invalid output pointers return `BLUSYS_ERR_INVALID_ARG`
- registering a callback before configuring a non-disabled interrupt mode returns `BLUSYS_ERR_INVALID_STATE`
- backend argument and state failures are translated into `blusys_err_t`

## Example App

See `examples/gpio_basic/`.
Its input and output pins are selected through example-local Kconfig options so board-safe GPIOs can be chosen per target board.

See `examples/gpio_interrupt/` for direct ISR callback usage.
