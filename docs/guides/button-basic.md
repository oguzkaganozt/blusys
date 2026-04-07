# Debounce a Button

## Problem Statement

You want to react to button presses and releases on an ESP32 without writing
debounce logic or timer management. A raw GPIO interrupt fires multiple times per
physical press due to contact bounce; you want clean PRESS / RELEASE / LONG_PRESS
events instead.

## Prerequisites

- Any supported board (ESP32, ESP32-C3, or ESP32-S3)
- A tactile button wired between a GPIO pin and GND — no external resistor needed
  when using the internal pull-up

## Minimal Example

```c
#include "blusys/blusys_all.h"

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

## APIs Used

- `blusys_button_open()` — configures the GPIO, installs the ANY_EDGE interrupt, and creates debounce/long-press timers
- `blusys_button_set_callback()` — registers the event handler; safe to call or replace while events are running
- `blusys_button_close()` — stops timers, removes the interrupt, frees the handle
- `blusys_button_read()` — reads instantaneous (un-debounced) pin state, useful for checking the initial state at startup

## Expected Runtime Behavior

- Pressing the button logs `pressed` after `debounce_ms` (default 50 ms)
- Releasing logs `released`
- Holding the button for `long_press_ms` (default 1000 ms) logs `long press!` while still held, followed by `released` on release
- Rapid bounce on press/release is suppressed — only one PRESS and one RELEASE event fires per physical gesture

## Common Mistakes

**Floating pin** — not setting a pull mode leaves the GPIO floating, causing spurious events even when the button is not touched. Always set `pull_mode` to `BLUSYS_GPIO_PULL_UP` (button to GND) or `BLUSYS_GPIO_PULL_DOWN` (button to VCC).

**Wrong `active_low`** — if PRESS and RELEASE are swapped, flip `active_low`. Buttons wired to GND are active-low (`active_low = true`). Buttons wired to VCC are active-high (`active_low = false`).

**Calling `blusys_button_close()` from the callback** — the close function spinwaits for the in-flight callback to finish, which deadlocks if called from inside that same callback. Schedule close from a separate task instead.

**Treating `blusys_button_read()` as debounced** — `read()` returns the raw GPIO level. Use the event callback for reliable edge detection.

## Example App

See `examples/button_basic/` for a complete runnable project.

Configure the pin via `idf.py menuconfig` → *Blusys Button Basic Example* and
flash with `blusys run examples/button_basic <port>`.

## API Reference

For full type definitions and function signatures, see [Button API Reference](../modules/button.md).
