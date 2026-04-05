# DAC

## Purpose

The `dac` module provides a small blocking oneshot output API for the on-chip DAC:

- open one DAC-capable GPIO
- write an 8-bit output value
- close the handle when finished

The first release is intentionally limited to the direct voltage-output path and hides ESP-IDF DAC channel types.

## Supported Targets

- ESP32

On `ESP32-C3` and `ESP32-S3`, the public API is present but returns `BLUSYS_ERR_NOT_SUPPORTED`, and `blusys_target_supports(BLUSYS_FEATURE_DAC)` reports `false`.

## Quick Start Example

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

## Lifecycle Model

DAC is handle-based:

1. call `blusys_dac_open()`
2. call `blusys_dac_write()` one or more times
3. call `blusys_dac_close()` when finished

## Blocking APIs

- `blusys_dac_open()`
- `blusys_dac_close()`
- `blusys_dac_write()`

## Thread Safety

- concurrent calls using the same handle are serialized internally
- do not call `blusys_dac_close()` concurrently with other calls using the same handle

## Limitations

- the first release supports only oneshot voltage output
- valid public pins are GPIO25 and GPIO26 on ESP32
- each DAC channel can be opened only once at a time across the whole process
- cosine generation and continuous DMA output are out of scope for the first release

## Error Behavior

- invalid pointers and non-DAC GPIOs return `BLUSYS_ERR_INVALID_ARG`
- opening an already-open DAC channel returns `BLUSYS_ERR_BUSY`
- calling `blusys_dac_write()` after close has started returns `BLUSYS_ERR_INVALID_STATE`
- unsupported targets return `BLUSYS_ERR_NOT_SUPPORTED`
- backend allocation and driver failures are translated into `blusys_err_t`

## Example App

See `examples/dac_basic/`.
