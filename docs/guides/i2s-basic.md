# Play Stereo Audio Over I2S

## Problem Statement

You want to stream simple stereo 16-bit audio data to an external I2S DAC or codec through a small common Blusys API.

## Prerequisites

- a supported board
- an external I2S DAC or codec wired to BCLK, WS, and DOUT
- optional MCLK wiring only if the external device requires it

## Minimal Example

```c
#include <stdint.h>

#include "blusys/i2s.h"

void app_main(void)
{
    blusys_i2s_tx_t *i2s;
    int16_t audio[] = {12000, 12000, -12000, -12000};

    if (blusys_i2s_tx_open(&(blusys_i2s_tx_config_t) {
                               .port = 0,
                               .bclk_pin = 4,
                               .ws_pin = 5,
                               .dout_pin = 6,
                               .mclk_pin = -1,
                               .sample_rate_hz = 16000u,
                           },
                           &i2s) != BLUSYS_OK) {
        return;
    }

    blusys_i2s_tx_start(i2s);
    blusys_i2s_tx_write(i2s, audio, sizeof(audio), 1000);
    blusys_i2s_tx_stop(i2s);
    blusys_i2s_tx_close(i2s);
}
```

## APIs Used

- `blusys_i2s_tx_open()` creates one transmit handle in standard master mode
- `blusys_i2s_tx_start()` enables BCLK and WS output
- `blusys_i2s_tx_write()` sends interleaved stereo 16-bit samples
- `blusys_i2s_tx_stop()` disables the transmit channel
- `blusys_i2s_tx_close()` releases the handle

## Expected Runtime Behavior

- the first release outputs standard Philips-format stereo audio
- `blusys_i2s_tx_write()` blocks until the driver accepts the requested buffer or the timeout expires
- if `mclk_pin` is `-1`, no MCLK signal is driven

## Common Mistakes

- expecting useful analog output without an external I2S DAC or codec
- wiring the external device to a different sample format than stereo 16-bit Philips mode
- calling `blusys_i2s_tx_write()` before `blusys_i2s_tx_start()`
- assuming RX or full-duplex support exists in the first release

## Validation Tips

- start with a modest sample rate such as `16000`
- keep the generated test pattern simple during bring-up so clock and wiring issues are easier to spot
- if audio is distorted, verify the external device does not require MCLK or configure `mclk_pin` when it does

## Example App

See `examples/i2s_basic/`.
