# Send A Pulse Sequence

## Problem Statement

You want to generate a timed digital waveform on one GPIO using a simple list of high and low pulse durations.

## Prerequisites

- a supported board
- one board-safe output GPIO
- optional: a scope, logic analyzer, or jumper to another measurement input

## Minimal Example

```c
#include "blusys/rmt.h"

void app_main(void)
{
    static const blusys_rmt_pulse_t pulses[] = {
        {.level = true, .duration_us = 1000u},
        {.level = false, .duration_us = 1000u},
        {.level = true, .duration_us = 500u},
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

## APIs Used

- `blusys_rmt_open()` creates one TX handle on one GPIO
- `blusys_rmt_write()` sends a pulse sequence and waits for completion
- `blusys_rmt_close()` releases the handle

## Expected Runtime Behavior

- the configured output pin toggles according to the pulse list
- the function returns when the sequence is finished
- the idle output level is restored after the transfer completes

## Common Mistakes

- choosing a GPIO that is not safe for output on the current board
- using zero-length pulse durations
- expecting RX, callbacks, or protocol decoders from the first `rmt` release

## Validation Tips

- use a scope or logic analyzer to verify timings
- on `esp32` and `esp32s3`, you can also jumper the RMT output into a `pcnt` input for simple pulse counting

## Example App

See `examples/rmt_basic/`.


## API Reference

For full type definitions and function signatures, see [RMT API Reference](../modules/rmt.md).
