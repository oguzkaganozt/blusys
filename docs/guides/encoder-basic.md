# Read a Rotary Encoder

## Problem Statement

You want to detect rotation direction and button presses from an EC11-style rotary
encoder without dealing with quadrature decoding, debouncing, or ISR management.

## Prerequisites

- Any supported board (ESP32, ESP32-C3, or ESP32-S3)
- An EC11 rotary encoder with CLK (A), DT (B), and optionally SW (button) wired to GPIO pins, common to GND

## Minimal Example

```c
#include "blusys/blusys.h"
#include "blusys/input/encoder.h"

static void on_event(blusys_encoder_t *enc, blusys_encoder_event_t event,
                      int position, void *ctx)
{
    (void)enc; (void)ctx;
    if (event == BLUSYS_ENCODER_EVENT_CW)    printf("CW  pos=%d\n", position);
    if (event == BLUSYS_ENCODER_EVENT_CCW)   printf("CCW pos=%d\n", position);
    if (event == BLUSYS_ENCODER_EVENT_PRESS) printf("pressed\n");
}

void app_main(void)
{
    blusys_encoder_t *enc;
    const blusys_encoder_config_t cfg = {
        .clk_pin          = 5,    /* change to your board's pins */
        .dt_pin           = 6,
        .sw_pin           = 7,    /* -1 to disable button */
        .glitch_filter_ns = 1000,
        .long_press_ms    = 1000,
    };

    blusys_encoder_open(&cfg, &enc);
    blusys_encoder_set_callback(enc, on_event, NULL);

    while (true) { vTaskDelay(portMAX_DELAY); }
}
```

## APIs Used

- `blusys_encoder_open()` — sets up PCNT quadrature decoding (ESP32/S3) or GPIO interrupt fallback (ESP32-C3), optionally initializes the push button
- `blusys_encoder_set_callback()` — registers the event handler; safe to call or replace while events are running
- `blusys_encoder_get_position()` — reads the accumulated detent position (CW increments, CCW decrements)
- `blusys_encoder_reset_position()` — resets the position counter to zero
- `blusys_encoder_close()` — stops decoding, removes interrupts, frees the handle

## Expected Runtime Behavior

- Rotating clockwise fires `CW` events with incrementing position
- Rotating counter-clockwise fires `CCW` events with decrementing position
- Pressing the button logs `PRESS`, releasing logs `RELEASE`
- Holding the button for `long_press_ms` fires `LONG_PRESS` while still held
- On ESP32/S3, PCNT hardware handles quadrature decoding with optional glitch filtering
- On ESP32-C3, GPIO interrupts with a Gray code state machine decode direction

## Common Mistakes

**Swapped CLK and DT** — if CW and CCW are reversed, swap the CLK and DT pin assignments in the config.

**Floating pins** — EC11 encoders need pull-ups on CLK, DT, and SW. The module enables internal pull-ups automatically. If using long wires, add external 10k pull-ups.

**Wrong `steps_per_detent`** — most EC11 encoders produce 4 quadrature pulses per detent (default). Some produce 2 or 1. If you get multiple events per click, increase this value. If you need to turn past a click, decrease it.

**Calling `blusys_encoder_close()` from the callback** — the close function waits for the in-flight callback to finish, which deadlocks if called from inside the callback.

## Example App

See `examples/encoder_basic/` for a complete runnable project.

Configure the pins via `idf.py menuconfig` → *Blusys Encoder Basic Example* and
flash with `blusys run examples/encoder_basic <port>`.

## API Reference

For full type definitions and function signatures, see [Encoder API Reference](../modules/encoder.md).
