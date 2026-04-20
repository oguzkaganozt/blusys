# Touch

Capacitive touch sensor: open one touch-capable GPIO and read filtered touch values.

Only one touch handle may be open at a time across the whole process.

## Quick Example

```c
#include <stdint.h>
#include "blusys/blusys.h"

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

## Common Mistakes

- picking a GPIO that is not touch-capable on the current target
- expecting multiple touch handles to be open at the same time in the first release
- comparing absolute readings across different boards as if they were calibrated units
- expecting threshold events or wake-from-sleep behavior from the first `touch` release

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | no |
| ESP32-S3 | yes |

On ESP32-C3, all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_TOUCH)` to check at runtime.

## Thread Safety

- concurrent calls on the same handle are serialized internally
- do not call `blusys_touch_close()` concurrently with other calls on the same handle

## Limitations

- one touch GPIO per handle; only one handle can be open at a time across the whole process
- the returned value is a filtered reading; its magnitude is target- and board-dependent
- threshold configuration, active/inactive events, wakeup, waterproofing, and proximity sensing are not exposed

## Example App

See `examples/validation/hal_io_lab` (touch scenario).
