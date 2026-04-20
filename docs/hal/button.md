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

**Floating pin** — not setting a pull mode leaves the GPIO floating, causing spurious events. Always set `pull_mode` to `BLUSYS_GPIO_PULL_UP` (button to GND) or `BLUSYS_GPIO_PULL_DOWN` (button to VCC).

**Wrong `active_low`** — if PRESS and RELEASE are swapped, flip `active_low`. Buttons wired to GND are active-low; buttons wired to VCC are active-high.

**Calling `blusys_button_close()` from the callback** — the close function spinwaits for the in-flight callback to finish, which deadlocks if called from inside that same callback. Schedule close from a separate task.

**Treating `blusys_button_read()` as debounced** — `read()` returns the raw GPIO level. Use the event callback for reliable edge detection.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Callback Context

Events fire from the **`esp_timer` task**, not from an ISR. It is safe to call `printf`, `ESP_LOGI`, FreeRTOS primitives, and most Blusys APIs from the callback. Do not call blocking operations that wait indefinitely.

## Thread Safety

- different button handles may be used concurrently from different tasks
- `blusys_button_set_callback()` is safe to call from any task while events are firing; it waits for any in-flight callback to complete before returning
- do **not** call `blusys_button_close()` from inside the button callback — this deadlocks on the callback-active spinwait

## ISR Notes

No ISR-safe calls are defined for the button module.

## Limitations

- one callback per handle; to fan out events, forward from the single callback
- `blusys_button_read()` returns an instantaneous GPIO value without debounce; use the callback for reliable edge detection
- long-press fires once per press; repeated long-press events while held are not generated

## Example App

See `examples/reference/hal` (button scenario).
