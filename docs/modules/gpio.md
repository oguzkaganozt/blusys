# GPIO

## Purpose

The `gpio` module provides a direct pin-based API for the most common digital IO tasks:

- reset a pin to its default state
- set input or output mode
- configure pull resistors
- read a pin level
- write a pin level

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

None in Phase 2.
Interrupt callbacks are deferred to a later phase.

## Thread Safety

- different pins may be controlled independently from different tasks
- Blusys does not coordinate concurrent access to the same pin
- read-modify-write patterns such as toggle are intentionally not part of this first API

## ISR Notes

Phase 2 does not promise ISR-safe GPIO calls.

## Limitations

- pin validation checks SoC-valid GPIO numbers, not board-safe wiring choices
- some boards reserve pins for flash, PSRAM, USB, JTAG, or onboard peripherals
- `blusys_gpio_read()` expects the pin to have been configured for input when meaningful input sampling is required

## Error Behavior

- invalid pins return `BLUSYS_ERR_INVALID_ARG`
- invalid output pointers return `BLUSYS_ERR_INVALID_ARG`
- backend argument and state failures are translated into `blusys_err_t`

## Example App

See `examples/gpio_basic/`.
Its input and output pins are selected through example-local Kconfig options so board-safe GPIOs can be chosen per target board.
