# ADC

Single-channel ADC input: raw conversion values and calibrated millivolt readings.

ADC is stateful and pin-based: open a channel, read from it, close it when done.

## Quick Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_adc_t *adc;
    int mv;

    if (blusys_adc_open(4, BLUSYS_ADC_ATTEN_DB_12, &adc) != BLUSYS_OK) {
        return;
    }

    if (blusys_adc_read_mv(adc, &mv) == BLUSYS_OK) {
        /* use mv */
    }

    blusys_adc_close(adc);
}
```

## Common Mistakes

- choosing a GPIO that is not ADC-capable on the selected target
- applying a voltage outside the safe range for the board
- assuming calibrated millivolt conversion is always available (some boards lack eFuse data)

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- concurrent reads on the same handle are serialized internally
- different handles may be used independently on different ADC1 channels
- the same channel cannot be opened by more than one handle at a time
- do not call `blusys_adc_close()` concurrently with other calls on the same handle

## Limitations

- limited to ADC1-backed GPIOs on the supported targets
- the public API is pin-based; ADC unit and channel IDs are not exposed
- reads are blocking and one-shot only
- ADC readings are approximate; noise depends on board layout and source impedance
- `blusys_adc_read_mv()` is only valid for attenuation modes with calibration support; `BLUSYS_ADC_ATTEN_DB_0` may return `BLUSYS_ERR_NOT_SUPPORTED` on some boards

## Example App

See `examples/reference/hal` (ADC scenario).
