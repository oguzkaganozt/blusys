# I2S

## Purpose

The `i2s` module provides a small blocking playback API for standard-mode I2S transmit on top of the ESP-IDF channel driver.

The first release is intentionally limited to a common subset that works across the supported targets without exposing ESP-IDF channel, slot, or clock types.

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include "blusys/i2s.h"

void app_main(void)
{
    blusys_i2s_tx_t *i2s;
    int16_t stereo_samples[] = {12000, 12000, -12000, -12000};

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
    blusys_i2s_tx_write(i2s, stereo_samples, sizeof(stereo_samples), 1000);
    blusys_i2s_tx_stop(i2s);
    blusys_i2s_tx_close(i2s);
}
```

## Configuration Model

- `port` selects the I2S controller
- `bclk_pin` drives bit clock
- `ws_pin` drives word select
- `dout_pin` drives TX sample data
- `mclk_pin` is optional and may be set to `-1`
- `sample_rate_hz` selects the playback sample rate

## Current Format Limits

- standard mode only
- master mode only
- TX only
- Philips format
- stereo only
- 16-bit samples only

Applications should provide interleaved left and right 16-bit samples in the write buffer.

## Lifecycle Model

I2S TX is handle-based:

1. call `blusys_i2s_tx_open()`
2. call `blusys_i2s_tx_start()`
3. call `blusys_i2s_tx_write()` one or more times
4. call `blusys_i2s_tx_stop()` when finished
5. call `blusys_i2s_tx_close()` to release resources

## Blocking APIs

- `blusys_i2s_tx_open()`
- `blusys_i2s_tx_close()`
- `blusys_i2s_tx_start()`
- `blusys_i2s_tx_stop()`
- `blusys_i2s_tx_write()`

## Thread Safety

- concurrent calls using the same handle are serialized internally
- do not call `blusys_i2s_tx_close()` concurrently with other calls using the same handle
- there is no async callback API in the first release

## Limitations

- an external I2S DAC or codec is required for useful playback output
- RX is not part of the first release
- mono, TDM, PDM, slave mode, and duplex are out of scope for the first release
- the API does not expose slot width, data width, or advanced clock tuning

## Error Behavior

- invalid pointers, invalid pins, invalid ports, and zero sample rate return `BLUSYS_ERR_INVALID_ARG`
- writing before `blusys_i2s_tx_start()` returns `BLUSYS_ERR_INVALID_STATE`
- backend allocation and driver failures are translated into `blusys_err_t`
- write timeout returns `BLUSYS_ERR_TIMEOUT`

## Example App

See `examples/i2s_basic/`.
