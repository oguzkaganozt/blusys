# Read A Capacitive Touch Pad

## Problem Statement

You want to poll one capacitive touch GPIO through a small common Blusys API and watch the filtered reading change while you touch and release the pad.

## Prerequisites

- a supported board
- one touch-capable GPIO exposed by that board
- a stable touch pad or conductive area wired to the selected touch GPIO

This guide applies to `ESP32` and `ESP32-S3`. On `ESP32-C3`, `touch` is reported as unsupported.

## Minimal Example

```c
#include <stdint.h>

#include "blusys/touch.h"

void app_main(void)
{
    blusys_touch_t *touch;
    uint32_t value;

    if (blusys_touch_open(4, &touch) != BLUSYS_OK) {
        return;
    }

    while (blusys_touch_read(touch, &value) == BLUSYS_OK) {
        (void) value;
    }

    blusys_touch_close(touch);
}
```

## APIs Used

- `blusys_touch_open()` creates one touch handle and starts internal scanning
- `blusys_touch_read()` returns the current filtered reading for that touch GPIO
- `blusys_touch_close()` stops scanning and releases the handle

## Expected Runtime Behavior

- the returned value updates while the background scan runs
- the absolute number depends on the target, board layout, and pad design
- touching and releasing the pad should produce a consistent change in the reported value

## Common Mistakes

- picking a GPIO that is not touch-capable on the current target
- expecting multiple touch handles to be open at the same time in the first release
- comparing absolute readings across different boards as if they were calibrated units
- expecting threshold events or wake-from-sleep behavior from the first `touch` release

## Validation Tips

- start with the example defaults for your target
- keep the pad isolated from noisy jumper wires during bring-up
- compare untouched and touched readings on the same board instead of relying on one universal threshold

## Example App

See `examples/touch_basic/`.
