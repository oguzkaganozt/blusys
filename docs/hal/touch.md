# Touch

Capacitive touch sensor: open one touch-capable GPIO and read filtered touch values.

## Quick Example

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

## Types

### `blusys_touch_t`

```c
typedef struct blusys_touch blusys_touch_t;
```

Opaque handle returned by `blusys_touch_open()`.

## Functions

### `blusys_touch_open`

```c
blusys_err_t blusys_touch_open(int pin, blusys_touch_t **out_touch);
```

Enables the touch controller and begins background scanning for the given pin. Only one touch handle may be open at a time.

**Parameters:**
- `pin` — touch-capable GPIO number (target-dependent)
- `out_touch` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for non-touch GPIOs or NULL pointer, `BLUSYS_ERR_BUSY` if another touch handle is already open, `BLUSYS_ERR_NOT_SUPPORTED` on ESP32-C3.

---

### `blusys_touch_close`

```c
blusys_err_t blusys_touch_close(blusys_touch_t *touch);
```

Stops background scanning and frees the handle.

---

### `blusys_touch_read`

```c
blusys_err_t blusys_touch_read(blusys_touch_t *touch, uint32_t *out_value);
```

Returns the current filtered touch reading. A higher value typically indicates stronger touch contact, but the absolute scale is target- and board-dependent.

**Parameters:**
- `touch` — handle
- `out_value` — output pointer for the touch reading

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if called after close has started, `BLUSYS_ERR_INVALID_ARG` if `out_value` is NULL.

## Lifecycle

1. `blusys_touch_open()` — enable controller and start scanning
2. `blusys_touch_read()` — poll touch value
3. `blusys_touch_close()` — stop and release

## Thread Safety

- concurrent calls on the same handle are serialized internally
- do not call `blusys_touch_close()` concurrently with other calls on the same handle

## Limitations

- one touch GPIO per handle; only one handle can be open at a time across the whole process
- the returned value is a filtered reading; its magnitude is target- and board-dependent
- threshold configuration, active/inactive events, wakeup, waterproofing, and proximity sensing are not exposed

## Example App

See `examples/validation/touch_basic/`.
