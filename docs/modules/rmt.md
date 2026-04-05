# RMT

## Purpose

The `rmt` module provides a small TX-only API for sending timed pulse sequences on one GPIO.

The public API hides the ESP-IDF RMT channel and encoder model and exposes a simpler pulse list in microseconds.

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include "blusys/rmt.h"

void app_main(void)
{
    static const blusys_rmt_pulse_t pulses[] = {
        {.level = true, .duration_us = 1000u},
        {.level = false, .duration_us = 1000u},
    };
    blusys_rmt_t *rmt;

    if (blusys_rmt_open(4, false, &rmt) != BLUSYS_OK) {
        return;
    }

    blusys_rmt_write(rmt, pulses, 2u, BLUSYS_TIMEOUT_FOREVER);
    blusys_rmt_close(rmt);
}
```

## Lifecycle Model

RMT is handle-based:

1. call `blusys_rmt_open()`
2. call `blusys_rmt_write()` to send one pulse sequence
3. call `blusys_rmt_close()` when finished

## Blocking APIs

- `blusys_rmt_open()`
- `blusys_rmt_close()`
- `blusys_rmt_write()`

## Thread Safety

- concurrent calls using the same handle are serialized internally
- do not call `blusys_rmt_close()` concurrently with other calls using the same handle

## Pulse Model

- each pulse contains a level and a duration in microseconds
- durations must be greater than zero
- the current implementation uses a fixed internal resolution of `1 MHz`
- each pulse duration must fit in the current internal hardware symbol limit

## Limitations

- TX only
- one output pin per handle
- no callbacks or async transmit yet
- no carrier modulation
- no protocol helpers
- no RX or capture API yet

## Error Behavior

- invalid pointers, invalid pins, empty pulse lists, and invalid durations return `BLUSYS_ERR_INVALID_ARG`
- closing while another call is active is prevented by the handle lock
- timeout while waiting for TX completion returns `BLUSYS_ERR_TIMEOUT`
- backend allocation and control failures are translated into `blusys_err_t`

## Example App

See `examples/rmt_basic/`.
