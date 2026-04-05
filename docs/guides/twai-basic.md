# Send And Receive TWAI Frames

## Problem Statement

You want to send classic TWAI/CAN frames and receive incoming frames through a small common Blusys API.

## Prerequisites

- a supported board
- an external TWAI transceiver wired to TX and RX pins
- a live bus or another active TWAI/CAN node

## Minimal Example

```c
#include "blusys/twai.h"

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

## APIs Used

- `blusys_twai_open()` creates one TWAI node handle
- `blusys_twai_set_rx_callback()` registers an ISR-context receive callback
- `blusys_twai_start()` enables the node on the bus
- `blusys_twai_write()` transmits one classic frame and waits for completion
- `blusys_twai_stop()` disables the node
- `blusys_twai_close()` releases the handle

## Expected Runtime Behavior

- outgoing writes block until the frame completes or times out
- incoming frames are delivered through the registered callback
- the first release is limited to standard classic frames with up to `8` bytes of payload

## Common Mistakes

- expecting the ESP32 pin alone to drive a CAN bus without an external transceiver
- forgetting to call `blusys_twai_start()` before writing
- doing blocking or allocation-heavy work inside the RX callback
- expecting extended identifiers from the first `twai` release
- expecting public filter or bus-off recovery controls from the first `twai` release

## Validation Tips

- verify bitrate and bus termination match the rest of the network
- start with a slow classic bitrate such as `125000` while bringing up hardware
- if you only have one board, use another node or CAN analyzer to verify traffic on the bus

## Example App

See `examples/twai_basic/`.
