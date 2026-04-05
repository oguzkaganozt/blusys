# SPI Master

## Purpose

The `spi` module provides a blocking full-duplex SPI master API for the common single-device path:

- open an SPI bus and one attached device
- transfer bytes
- close the SPI handle

Phase 3 intentionally keeps the device configuration small and fixes the mode to SPI mode 0.

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include <stdint.h>

#include "blusys/spi.h"

void app_main(void)
{
    blusys_spi_t *spi;
    uint8_t tx_data[2] = {0xaa, 0x55};
    uint8_t rx_data[2] = {0};

    if (blusys_spi_open(0, 4, 6, 5, 7, 1000000, &spi) != BLUSYS_OK) {
        return;
    }

    blusys_spi_transfer(spi, tx_data, rx_data, sizeof(tx_data));
    blusys_spi_close(spi);
}
```

## Lifecycle Model

SPI is handle-based:

1. call `blusys_spi_open()`
2. use `blusys_spi_transfer()`
3. call `blusys_spi_close()` when finished

The handle owns both the SPI bus initialization and one attached SPI device.

## Blocking APIs

- `blusys_spi_open()`
- `blusys_spi_close()`
- `blusys_spi_transfer()`

## Async APIs

None in Phase 3.

## Thread Safety

- concurrent transfers on the same handle are serialized internally
- different SPI handles may be used independently when they use different SPI buses
- do not call `blusys_spi_close()` concurrently with other calls using the same handle

## ISR Notes

Phase 3 does not define ISR-safe SPI calls.

## Limitations

- SPI mode 0 is fixed in Phase 3
- one Blusys SPI handle owns one SPI bus and one chip-select line
- advanced queueing and multi-device bus sharing are intentionally deferred

## Error Behavior

- invalid bus indices, pins, frequency, or pointers return `BLUSYS_ERR_INVALID_ARG`
- trying to open an SPI bus that is already owned elsewhere returns `BLUSYS_ERR_INVALID_STATE`
- transfer failures reported by the backend are translated into `blusys_err_t`

## Example App

See `examples/spi_loopback/`.
