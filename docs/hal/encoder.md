# Encoder

Rotary encoder driver with hardware quadrature decoding where available, plus an optional push-button.

## At a glance

- PCNT hardware on ESP32 and ESP32-S3
- GPIO fallback on ESP32-C3
- events fire from the esp_timer task

## Quick example

```c
static void on_event(blusys_encoder_t *enc, blusys_encoder_event_t event,
                     int position, void *ctx)
{
    (void)enc; (void)ctx;
    if (event == BLUSYS_ENCODER_EVENT_CW) printf("CW %d\n", position);
}

blusys_encoder_t *enc;
blusys_encoder_config_t cfg = {
    .clk_pin = 5,
    .dt_pin = 6,
    .sw_pin = 7,
};
blusys_encoder_open(&cfg, &enc);
blusys_encoder_set_callback(enc, on_event, NULL);
```

## Common mistakes

- swapping CLK and DT
- forgetting pull-ups on long wires
- using the wrong `steps_per_detent`
- closing from inside the callback

## Target support

| Target | Supported | Decoding |
|--------|-----------|----------|
| ESP32 | yes | PCNT hardware |
| ESP32-C3 | yes | GPIO software |
| ESP32-S3 | yes | PCNT hardware |

## Callback context

- events fire from the esp_timer task
- `get_position()` and `reset_position()` are safe from any task

## Limitations

- one callback per handle
- GPIO fallback may miss very fast rotation

## Example app

`examples/reference/display`
