# RMT

Timed digital pulse sequences: TX for generating IR codes and similar protocols, RX for capturing pulses.

Internal resolution is 1 MHz (1 µs per tick). TX and RX are independent handles.

## Quick Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    static const blusys_rmt_pulse_t pulses[] = {
        {.level = true,  .duration_us = 1000u},
        {.level = false, .duration_us = 1000u},
        {.level = true,  .duration_us = 500u},
        {.level = false, .duration_us = 500u},
    };
    blusys_rmt_t *rmt;

    if (blusys_rmt_open(4, false, &rmt) != BLUSYS_OK) {
        return;
    }

    blusys_rmt_write(rmt, pulses, 4u, BLUSYS_TIMEOUT_FOREVER);
    blusys_rmt_close(rmt);
}
```

## Common Mistakes

- choosing a GPIO that is not safe for output on the current board
- using zero-length pulse durations
- expecting callbacks, carrier modulation, or protocol decoders from the current `rmt` release

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread Safety

- concurrent calls on the same handle are serialized internally
- TX and RX handles are independent
- do not call close concurrently with other calls on the same handle

## Limitations

- pulse resolution is fixed at 1 MHz (1 µs per tick)
- no carrier modulation, protocol helpers, or async transmit
- one input/output pin per handle

## Example App

See `examples/validation/peripheral_lab/` (`periph_rmt` scenario — menuconfig: TX or RX mode).
