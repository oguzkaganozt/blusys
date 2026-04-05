# Sigma-Delta Modulation

## Purpose

The `sdm` module provides a sigma-delta modulation output on a GPIO pin:

- open an SDM channel on a pin with a sample rate
- set the pulse density to produce a pseudo-analog output level
- close the handle

SDM is useful for driving LEDs, simple DACs with an RC filter, or any load where a PWM-style bit-stream is acceptable.

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include "blusys/sdm.h"

void app_main(void)
{
    blusys_sdm_t *sdm;

    if (blusys_sdm_open(8, 1000000, &sdm) != BLUSYS_OK) {
        return;
    }

    blusys_sdm_set_density(sdm, 64);   /* ~75% output level */
    blusys_sdm_close(sdm);
}
```

## Lifecycle Model

SDM is handle-based:

1. call `blusys_sdm_open()` — configures the channel and enables the output
2. call `blusys_sdm_set_density()` to change the output level
3. call `blusys_sdm_close()` when finished

## Density Scale

The `density` parameter is a signed 8-bit value in the range `[-128, 127]`:

| density | Approximate output |
|---------|--------------------|
| -128 | ~0% (always low) |
| 0 | ~50% |
| 127 | ~100% (always high) |

The output voltage after an RC low-pass filter tracks linearly with density. A 4.7 kΩ + 100 nF RC filter is a common choice for audio-frequency analog approximation.

## Blocking APIs

- `blusys_sdm_open()`
- `blusys_sdm_close()`
- `blusys_sdm_set_density()`

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_sdm_close()` concurrently with other calls using the same handle

## ISR Notes

No ISR-safe calls are defined for the SDM module.

## Limitations

- one channel per handle; each channel occupies one GPIO and one hardware SDM channel slot
- the output is a high-frequency digital signal; an RC filter is required for analog use
- SDM is distinct from LEDC PWM — SDM uses fixed frequency with variable duty density; LEDC uses configurable frequency with duty control

## Error Behavior

- invalid pin, zero sample rate, or null pointer return `BLUSYS_ERR_INVALID_ARG`
- driver initialization failures return the translated ESP-IDF error

## Example App

See `examples/sdm_basic/`.
