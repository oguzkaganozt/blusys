# TWAI

## Purpose

The `twai` module provides a small classic CAN-style API for sending and receiving standard TWAI frames through the ESP-IDF TWAI node driver.

The public API keeps the common path handle-based and hides the underlying ESP-IDF node, frame, and callback types.

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

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
        .dlc = 2u,
        .data = {0x11u, 0x22u},
        .data_len = 2u,
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

## Lifecycle Model

TWAI is handle-based:

1. call `blusys_twai_open()`
2. optionally register `blusys_twai_set_rx_callback()`
3. call `blusys_twai_start()` to join the bus
4. call `blusys_twai_write()` to send frames
5. call `blusys_twai_stop()` when finished with the bus
6. call `blusys_twai_close()` to release resources

## Frame Model

- standard identifiers are supported in this first release
- classic frames only in this first release
- payload length must be between `0` and `8` bytes
- `dlc` is explicit so remote frames can carry a classic request length
- remote frames must use `data_len == 0`
- CAN FD is intentionally out of scope

## Blocking APIs

- `blusys_twai_open()`
- `blusys_twai_close()`
- `blusys_twai_start()`
- `blusys_twai_stop()`
- `blusys_twai_write()`
- `blusys_twai_set_rx_callback()`

## Thread Safety

- concurrent calls using the same handle are serialized internally
- do not call `blusys_twai_close()` concurrently with other calls using the same handle
- the RX callback runs in ISR context and must stay ISR-safe

## Limitations

- external transceiver hardware is required
- receive uses a callback; there is no blocking `read()` API yet
- only classic frames are supported
- no public filter, loopback, listen-only, self-test, or bus-off recovery API yet
- no explicit status or error callback API yet

## Error Behavior

- invalid pointers, invalid pins, invalid bitrates, non-standard frame IDs, oversized payloads, invalid `dlc`, and remote frames with data return `BLUSYS_ERR_INVALID_ARG`
- writing before `blusys_twai_start()` returns `BLUSYS_ERR_INVALID_STATE`
- timeout while waiting for TX completion returns `BLUSYS_ERR_TIMEOUT`
- a completed transmit that the backend reports as failed returns `BLUSYS_ERR_IO`
- backend allocation and control failures are translated into `blusys_err_t`

## Example App

See `examples/twai_basic/`.
