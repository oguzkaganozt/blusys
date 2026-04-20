# Encoder

Rotary encoder driver with hardware quadrature decoding (PCNT on ESP32 / ESP32-S3) and GPIO interrupt fallback (ESP32-C3), plus an optional push-button.

## Quick Example

```c
#include "blusys/blusys.h"

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

## Common Mistakes

**Swapped CLK and DT** — if CW and CCW are reversed, swap the CLK and DT pin assignments in the config.

**Floating pins** — EC11 encoders need pull-ups on CLK, DT, and SW. The driver enables internal pull-ups automatically; if using long wires, add external 10 kΩ pull-ups.

**Wrong `steps_per_detent`** — most EC11 encoders produce 4 quadrature pulses per detent (default). Some produce 2 or 1. If you get multiple events per click, increase this value; if you need to turn past a click, decrease it.

**Calling `blusys_encoder_close()` from the callback** — the close function waits for the in-flight callback to finish, which deadlocks if called from inside the callback.

## Target Support

| Target | Supported | Decoding |
|--------|-----------|----------|
| ESP32 | yes | PCNT hardware |
| ESP32-C3 | yes | GPIO software |
| ESP32-S3 | yes | PCNT hardware |

## Callback Context

Events fire from the **`esp_timer` task**, not from an ISR. Safe to call `printf`, `ESP_LOGI`, FreeRTOS primitives, and most Blusys APIs. Do not call blocking operations that wait indefinitely.

## Thread Safety

- different encoder handles may be used concurrently from different tasks
- `blusys_encoder_set_callback()` waits for any in-flight callback to complete before returning
- `blusys_encoder_get_position()` and `blusys_encoder_reset_position()` are safe to call from any task at any time
- do **not** call `blusys_encoder_close()` from inside the encoder callback

## ISR Notes

No ISR-safe calls are defined for the encoder module.

## Limitations

- one callback per handle; to fan out events, forward from the single callback
- the GPIO fallback (ESP32-C3) may miss edges at very high rotation speeds compared to the hardware PCNT path
- `glitch_filter_ns` is only effective on ESP32 and ESP32-S3; it is ignored on ESP32-C3
- position wraps at `int` limits
- long-press fires once per press; repeated long-press events while held are not generated

## Example App

See `examples/reference/display` (encoder scenario).
