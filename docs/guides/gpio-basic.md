# Basic GPIO IO

## Problem Statement

You want to configure one pin as an output, one pin as an input, and use a short API to read and write levels.

## Prerequisites

- a board with two user-selectable GPIO pins
- one safe output pin for your board
- one safe input pin for your board
- set `Blusys GPIO Basic Example -> Output GPIO pin` and `Input GPIO pin` in `idf.py menuconfig` before flashing if the defaults are not safe for your board

## Minimal Example

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

## Explanation

- `blusys_gpio_reset()` returns the pin to a default state before applying a new role
- `blusys_gpio_set_output()` and `blusys_gpio_set_input()` keep direction setup explicit
- `blusys_gpio_set_pull_mode()` keeps pull resistor control separate from direction
- `blusys_gpio_read()` and `blusys_gpio_write()` use `bool` levels for the common path

## Expected Runtime Behavior

The output pin follows the level you write and the input pin reports the sampled logic level.

## Common Mistakes

- choosing pins that are valid on the SoC but reserved on the specific board
- writing to a pin that was not configured for output
- reading a floating input without setting an external or internal pull resistor

## Example App

See `examples/gpio_basic/` for a runnable example.
The example uses Kconfig-selected pins so you can choose board-safe GPIOs without editing source.
