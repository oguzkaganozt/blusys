# Touch

## Purpose

The `touch` module provides a small handle-based API for one capacitive touch GPIO:

- open one touch-capable pin
- read a filtered touch value
- close the handle when finished

The public API hides the ESP-IDF controller, channel, and touch-hardware-version details behind one blocking read path.

## Supported Targets

- ESP32
- ESP32-S3

On `ESP32-C3`, the public API is present but returns `BLUSYS_ERR_NOT_SUPPORTED`, and `blusys_target_supports(BLUSYS_FEATURE_TOUCH)` reports `false`.

## Quick Start Example

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

    blusys_touch_read(touch, &value);
    blusys_touch_close(touch);
}
```

## Lifecycle Model

Touch is handle-based:

1. call `blusys_touch_open()`
2. call `blusys_touch_read()` one or more times
3. call `blusys_touch_close()` when finished

`blusys_touch_open()` starts internal background scanning so `blusys_touch_read()` can return a filtered value without extra start or trigger calls.

## Blocking APIs

- `blusys_touch_open()`
- `blusys_touch_close()`
- `blusys_touch_read()`

## Thread Safety

- concurrent calls using the same handle are serialized internally
- do not call `blusys_touch_close()` concurrently with other calls using the same handle
- there is no callback API in the first release

## Limitations

- the public API is limited to one touch GPIO per handle
- the first release supports only one open touch handle at a time across the whole process
- the returned value is a filtered touch reading, and its absolute magnitude is target- and board-dependent
- threshold configuration, active or inactive events, wake-up behavior, waterproofing, and proximity sensing are out of scope for the first release

## Error Behavior

- invalid pointers and non-touch GPIOs return `BLUSYS_ERR_INVALID_ARG`
- opening a second touch handle while one is already open returns `BLUSYS_ERR_BUSY`
- calling `blusys_touch_read()` after close has started returns `BLUSYS_ERR_INVALID_STATE`
- unsupported targets return `BLUSYS_ERR_NOT_SUPPORTED`
- backend allocation and driver failures are translated into `blusys_err_t`

## Example App

See `examples/touch_basic/`.
