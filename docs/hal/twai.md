# TWAI

Classic CAN-style communication using the ESP32's TWAI controller. Supports sending and receiving standard 11-bit frames with an async RX callback.

**Requires an external CAN transceiver.** The TWAI pins connect to the transceiver's TXD/RXD, not directly to the bus wires.

## Quick Example

```c
#include "blusys/blusys.h"

static bool on_rx(blusys_twai_t *twai, const blusys_twai_frame_t *frame, void *user_ctx)
{
    (void) twai;
    (void) frame;
    (void) user_ctx;
    return false;
}

void app_main(void)
{
    blusys_twai_t *twai;
    blusys_twai_frame_t frame = {
        .id = 0x123u,
        .dlc = 3u,
        .data = {0x10u, 0x20u, 0x30u},
        .data_len = 3u,
    };

    if (blusys_twai_open(4, 5, 125000u, &twai) != BLUSYS_OK) {
        return;
    }

    blusys_twai_set_rx_callback(twai, on_rx, NULL);
    blusys_twai_start(twai);
    blusys_twai_write(twai, &frame, 1000);
    blusys_twai_stop(twai);
    blusys_twai_close(twai);
}
```

## Common Mistakes

- expecting the ESP32 pin alone to drive a CAN bus without an external transceiver
- forgetting to call `blusys_twai_start()` before writing
- doing blocking or allocation-heavy work inside the RX callback
- expecting extended identifiers from the first `twai` release
- expecting public filter or bus-off recovery controls from the first `twai` release

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- concurrent calls on the same handle are serialized internally
- do not call `blusys_twai_close()` concurrently with other calls on the same handle
- the RX callback runs in ISR context

## Limitations

- external transceiver hardware is required (the TWAI pins connect to transceiver TXD/RXD, not directly to the bus)
- receive uses a callback only; no blocking read API
- only classic standard frames (11-bit ID) are supported; CAN FD is out of scope
- no filter, loopback, listen-only, or bus-off recovery API

## Example App

See `examples/validation/peripheral_lab/` (`periph_twai` scenario).
