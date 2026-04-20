# DAC

On-chip digital-to-analog converter: write an 8-bit voltage level to a DAC-capable GPIO.

## Quick Example

```c
#include <stdint.h>
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_dac_t *dac;

    if (blusys_dac_open(25, &dac) != BLUSYS_OK) {
        return;
    }

    blusys_dac_write(dac, 128u);
    blusys_dac_close(dac);
}
```

## Common Mistakes

- trying to use DAC on ESP32-C3 or ESP32-S3 (not supported — check at runtime via `blusys_target_supports(BLUSYS_FEATURE_DAC)`)
- selecting a GPIO that is not DAC-capable
- expecting audio streaming, waveform generation, or calibrated voltage units (not in scope for the first DAC release)

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | no |
| ESP32-S3 | no |

## Thread Safety

- concurrent calls on the same handle are serialized internally
- do not call `blusys_dac_close()` concurrently with other calls on the same handle

## Limitations

- valid only on GPIO25 and GPIO26 (ESP32)
- each DAC channel can be opened only once at a time
- only direct oneshot voltage output is supported; cosine generation and continuous DMA output are not exposed

## Example App

See `examples/validation/hal_io_lab` (DAC scenario).
