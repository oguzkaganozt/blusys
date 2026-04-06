# Output A DAC Voltage

## Problem Statement

You want to drive the on-chip DAC with a simple 8-bit output value through a small common Blusys API.

## Prerequisites

- an ESP32 board
- access to GPIO25 or GPIO26
- a multimeter or oscilloscope for observing the analog output

On `ESP32-C3` and `ESP32-S3`, `dac` is reported as unsupported.

## Minimal Example

```c
#include <stdint.h>

#include "blusys/dac.h"

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

## APIs Used

- `blusys_dac_open()` creates one DAC handle for GPIO25 or GPIO26 on ESP32
- `blusys_dac_write()` writes the current 8-bit output value
- `blusys_dac_close()` releases the DAC channel

## Expected Runtime Behavior

- the output voltage changes when the written value changes
- the valid write range is `0..255`
- larger values produce higher output voltage within the DAC range

## Common Mistakes

- trying to use DAC on `ESP32-C3` or `ESP32-S3`
- selecting a GPIO that is not DAC-capable
- expecting audio streaming, waveform generation, or calibrated voltage units from the first DAC release

## Validation Tips

- start with a few fixed values such as `0`, `128`, and `255`
- use the `dac_basic` example ramp pattern to confirm the output changes visibly on a scope or meter
- avoid heavy external loading on the DAC pin during bring-up

## Example App

See `examples/dac_basic/`.


## API Reference

For full type definitions and function signatures, see [DAC API Reference](../modules/dac.md).
