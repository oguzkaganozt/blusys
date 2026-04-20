# I2S

Stereo audio streaming over I2S: TX for playback, RX for capture. Fixed to standard Philips mode, 16-bit stereo.

TX and RX are independent channels and cannot share a single port.

## Quick Example

```c
#include <stdint.h>
#include "blusys/blusys.h"

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

## Common Mistakes

- expecting useful analog output without an external I2S DAC or codec
- wiring the external device to a different sample format than stereo 16-bit Philips mode
- calling `blusys_i2s_tx_write()` before `blusys_i2s_tx_start()`
- assuming full-duplex support exists — TX and RX are separate channels and cannot share a single port

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- concurrent calls on the same handle are serialized internally
- TX and RX handles are independent
- do not call close concurrently with other calls on the same handle

## Limitations

- format is fixed: Philips standard mode, master mode, stereo, 16-bit samples
- an external I2S DAC or ADC/microphone is required
- mono, TDM, PDM, slave mode, and duplex are not supported
- slot width, data width, and advanced clock tuning are not exposed

## Example App

See `examples/validation/peripheral_lab/` (I2S scenario — menuconfig: TX or RX mode).
