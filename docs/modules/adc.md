# ADC

## Purpose

The `adc` module provides a blocking handle-based API for the common single-channel ADC path:

- open one ADC input on a GPIO pin
- read the raw conversion value
- read a calibrated millivolt value when calibration is available
- close the ADC handle

Phase 4 keeps the API intentionally smaller than the full ESP-IDF ADC driver surface and hides ADC unit and channel mapping internally.

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include "blusys/adc.h"

void app_main(void)
{
    blusys_adc_t *adc;
    int raw;

    if (blusys_adc_open(4, BLUSYS_ADC_ATTEN_DB_12, &adc) != BLUSYS_OK) {
        return;
    }

    if (blusys_adc_read_raw(adc, &raw) == BLUSYS_OK) {
        /* use raw */
    }

    blusys_adc_close(adc);
}
```

## Lifecycle Model

ADC is handle-based:

1. call `blusys_adc_open()`
2. call `blusys_adc_read_raw()` or `blusys_adc_read_mv()` as needed
3. call `blusys_adc_close()` when finished

## Blocking APIs

- `blusys_adc_open()`
- `blusys_adc_close()`
- `blusys_adc_read_raw()`
- `blusys_adc_read_mv()`

## Async APIs

None in Phase 4.

## Thread Safety

- concurrent calls using the same handle are serialized internally
- different ADC handles may be used independently on different ADC1 channels
- the same ADC1 channel cannot be opened by more than one handle at a time
- do not call `blusys_adc_close()` concurrently with other calls using the same handle

## ISR Notes

Phase 4 does not define ISR-safe ADC calls.

## Limitations

- the common v1 API is limited to ADC1-backed GPIOs on the supported ESP32, ESP32-C3, and ESP32-S3 targets
- the public API is pin-based and does not expose ADC unit or channel IDs
- reads are blocking and one-shot only
- calibrated millivolt conversion is only supported for the common calibrated attenuation modes and still depends on the target and board calibration data being usable
- ADC readings are approximate and may be noisy depending on the board and source impedance

## Error Behavior

- invalid pins, attenuation values, or pointers return `BLUSYS_ERR_INVALID_ARG`
- opening an ADC channel that is already owned by another Blusys ADC handle returns `BLUSYS_ERR_BUSY`
- attenuation modes without calibration support still allow `blusys_adc_read_raw()`, but `blusys_adc_read_mv()` returns `BLUSYS_ERR_NOT_SUPPORTED`
- `blusys_adc_read_mv()` returns `BLUSYS_ERR_NOT_SUPPORTED` when calibrated conversion is not available on the current board configuration
- backend ADC failures are translated into `blusys_err_t`

## Example App

See `examples/adc_basic/`.
