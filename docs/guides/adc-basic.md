# Read ADC Input

## Problem Statement

You want to read an analog input through the Blusys ADC API.

## Prerequisites

- a supported board
- one ADC-capable GPIO for your target and board
- a safe analog source to measure

## Minimal Example

```c
#include "blusys/adc.h"

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

## APIs Used

- `blusys_adc_open()` opens one ADC input by GPIO number
- `blusys_adc_read_raw()` returns the raw conversion value
- `blusys_adc_read_mv()` returns a calibrated millivolt reading when supported
- `blusys_adc_close()` releases the ADC handle

## Expected Runtime Behavior

- valid ADC pins open successfully
- raw reads return changing values as the input voltage changes
- millivolt reads work when calibration is available on the current target and board setup

## Common Mistakes

- choosing a GPIO that is not ADC-capable on the selected target
- applying a voltage outside the safe range for the board
- assuming calibrated millivolt conversion is always available

## Example App

See `examples/adc_basic/`.


## API Reference

For full type definitions and function signatures, see [ADC API Reference](../modules/adc.md).
