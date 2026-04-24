# GPIO

Pin-based digital IO: input, output, pull resistors, and direct ISR callbacks.

## At a glance

- stateless API, no handle
- configure the pin, use it, reset it when done
- callbacks run in ISR context

## Quick example

```c
bool level;
blusys_gpio_reset(3);
blusys_gpio_set_output(3);
blusys_gpio_write(3, true);
blusys_gpio_reset(2);
blusys_gpio_set_input(2);
blusys_gpio_set_pull_mode(2, BLUSYS_GPIO_PULL_UP);
blusys_gpio_read(2, &level);
```

## Common mistakes

- using board-reserved pins
- writing to an input pin
- reading a floating input

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- different pins may be controlled independently
- the API does not coordinate concurrent access to the same pin

## ISR notes

- callbacks run in ISR context
- they must not block
- return `true` to wake a higher-priority task

## Limitations

- pin validation checks the SoC, not the board
- one callback per pin

## Example apps

- `examples/reference/hal` (`GPIO`)
- `examples/validation/hal_io_lab` (`GPIO interrupt`)
