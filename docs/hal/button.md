# Button

GPIO-based button abstraction with software debounce and long-press detection.

## Quick Example

```c
#include "blusys/blusys.h"

static void on_button(blusys_button_t *btn, blusys_button_event_t event, void *ctx)
{
    (void)btn; (void)ctx;
    if (event == BLUSYS_BUTTON_EVENT_PRESS)      printf("pressed\n");
    if (event == BLUSYS_BUTTON_EVENT_RELEASE)    printf("released\n");
    if (event == BLUSYS_BUTTON_EVENT_LONG_PRESS) printf("long press!\n");
}

void app_main(void)
{
    blusys_button_t *btn;
    const blusys_button_config_t cfg = {
        .pin           = 2,                   /* change to your board's button pin */
        .pull_mode     = BLUSYS_GPIO_PULL_UP,
        .active_low    = true,
        .debounce_ms   = 50,
        .long_press_ms = 1000,
    };

    blusys_button_open(&cfg, &btn);
    blusys_button_set_callback(btn, on_button, NULL);

    while (true) { vTaskDelay(portMAX_DELAY); }
}
```

## Common Mistakes

**Floating pin** — not setting a pull mode leaves the GPIO floating, causing spurious events even when the button is not touched. Always set `pull_mode` to `BLUSYS_GPIO_PULL_UP` (button to GND) or `BLUSYS_GPIO_PULL_DOWN` (button to VCC).

**Wrong `active_low`** — if PRESS and RELEASE are swapped, flip `active_low`. Buttons wired to GND are active-low (`active_low = true`). Buttons wired to VCC are active-high (`active_low = false`).

**Calling `blusys_button_close()` from the callback** — the close function spinwaits for the in-flight callback to finish, which deadlocks if called from inside that same callback. Schedule close from a separate task instead.

**Treating `blusys_button_read()` as debounced** — `read()` returns the raw GPIO level. Use the event callback for reliable edge detection.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_button_event_t`

```c
typedef enum {
    BLUSYS_BUTTON_EVENT_PRESS,
    BLUSYS_BUTTON_EVENT_RELEASE,
    BLUSYS_BUTTON_EVENT_LONG_PRESS,
} blusys_button_event_t;
```

| Value | Meaning |
|-------|---------|
| `BLUSYS_BUTTON_EVENT_PRESS` | Button was pressed (after debounce) |
| `BLUSYS_BUTTON_EVENT_RELEASE` | Button was released (after debounce) |
| `BLUSYS_BUTTON_EVENT_LONG_PRESS` | Button has been held for `long_press_ms` |

### `blusys_button_callback_t`

```c
typedef void (*blusys_button_callback_t)(blusys_button_t *button,
                                         blusys_button_event_t event,
                                         void *user_ctx);
```

Callback signature for button events. Fires from the `esp_timer` task — safe to call blusys APIs, `printf`, or FreeRTOS functions. Must not block indefinitely.

### `blusys_button_config_t`

```c
typedef struct {
    int                pin;
    blusys_gpio_pull_t pull_mode;     /* BLUSYS_GPIO_PULL_UP recommended */
    bool               active_low;    /* true = pressed when GPIO is low (typical) */
    uint32_t           debounce_ms;   /* 0 → defaults to 50 ms */
    uint32_t           long_press_ms; /* 0 → long-press detection disabled */
} blusys_button_config_t;
```

---

## Functions

### `blusys_button_open`

```c
blusys_err_t blusys_button_open(const blusys_button_config_t *config,
                                 blusys_button_t **out_button);
```

Configures the GPIO pin as an input, installs an ANY_EDGE interrupt, and creates the internal debounce and long-press timers. Does not fire any callback until `blusys_button_set_callback()` is called.

**Parameters:**
- `config` — pin number, pull mode, active_low, debounce_ms, long_press_ms (required)
- `out_button` — output handle (required)

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if any pointer is NULL, `BLUSYS_ERR_NO_MEM` if allocation fails, `BLUSYS_ERR_FAIL` on driver error.

---

### `blusys_button_close`

```c
blusys_err_t blusys_button_close(blusys_button_t *button);
```

Disables the GPIO interrupt, stops and deletes the internal timers, waits for any in-flight callback to finish, then frees the handle. Safe to call from any task.

**Parameters:**
- `button` — handle returned by `blusys_button_open()` (required)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `button` is NULL.

---

### `blusys_button_set_callback`

```c
blusys_err_t blusys_button_set_callback(blusys_button_t *button,
                                         blusys_button_callback_t callback,
                                         void *user_ctx);
```

Registers (or replaces) the event callback. Pass `NULL` for `callback` to clear it. After this function returns, the old callback will not be called again.

**Parameters:**
- `button` — handle (required)
- `callback` — function to call on each event, or NULL to disable
- `user_ctx` — passed through to each callback invocation

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `button` is NULL, `BLUSYS_ERR_INVALID_STATE` if `blusys_button_close()` is in progress.

---

### `blusys_button_read`

```c
blusys_err_t blusys_button_read(blusys_button_t *button, bool *out_pressed);
```

Reads the current raw GPIO level and translates it to a logical pressed state according to `active_low`. No debounce is applied — this reflects the instantaneous pin state.

**Parameters:**
- `button` — handle (required)
- `out_pressed` — `true` if the button is currently pressed (required)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if any pointer is NULL, or a GPIO read error.

---

## Lifecycle

```
blusys_button_open()
    │
    ├── blusys_button_set_callback()   ← register event handler
    │
    ├── [events fire: PRESS / RELEASE / LONG_PRESS]
    │
    └── blusys_button_close()
```

## Thread Safety

- Different button handles may be used concurrently from different tasks.
- `blusys_button_set_callback()` is safe to call from any task while events are firing; it waits for any in-flight callback to complete before returning.
- Do **not** call `blusys_button_close()` from inside the button callback — this deadlocks on the callback_active spinwait.

## Callback Context

Events fire from the **esp_timer task**, not from an ISR. It is safe to call `printf`, `ESP_LOGI`, FreeRTOS primitives, and most blusys APIs from inside the callback. Do not call blocking operations that wait indefinitely.

## Limitations

- One callback per handle. To fan out events to multiple consumers, forward from the single callback.
- `blusys_button_read()` returns an instantaneous GPIO value without debounce; use the callback for reliable edge detection.
- Long-press fires once per press. Repeated long-press events while held are not generated.

## Example App

See `examples/reference/hal` (button scenario) for a runnable example.
