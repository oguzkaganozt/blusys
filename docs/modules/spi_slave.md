# SPI Slave

## Purpose

The `spi_slave` module provides a synchronous slave-mode SPI API:

- open an SPI slave with MOSI, MISO, CLK, CS pins and a clock mode
- perform a full-duplex transfer with a master (blocking with timeout)
- close the slave handle

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include <stdint.h>
#include <string.h>

#include "blusys/spi_slave.h"

void app_main(void)
{
    blusys_spi_slave_t *slave;
    uint8_t rx[16];
    uint8_t tx[16];

    if (blusys_spi_slave_open(11, 13, 12, 10, 0, &slave) != BLUSYS_OK) {
        return;
    }

    memset(tx, 0xAB, sizeof(tx));
    blusys_spi_slave_transfer(slave, tx, rx, sizeof(rx), 5000);
    blusys_spi_slave_close(slave);
}
```

## Lifecycle Model

SPI slave is handle-based:

1. call `blusys_spi_slave_open()` — initializes the host, pins, and clock mode
2. call `blusys_spi_slave_transfer()` to wait for a master transfer and exchange bytes
3. call `blusys_spi_slave_close()` when finished

## Clock Modes

| `mode` | CPOL | CPHA | Description |
|--------|------|------|-------------|
| 0 | 0 | 0 | clock idle low, sample on rising edge |
| 1 | 0 | 1 | clock idle low, sample on falling edge |
| 2 | 1 | 0 | clock idle high, sample on falling edge |
| 3 | 1 | 1 | clock idle high, sample on rising edge |

The `mode` parameter must match the SPI master configuration.

## Blocking APIs

- `blusys_spi_slave_open()`
- `blusys_spi_slave_close()`
- `blusys_spi_slave_transfer()` — blocks until the master initiates a transfer or timeout expires

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_spi_slave_close()` concurrently with other calls using the same handle

## ISR Notes

No ISR-safe calls are defined for the SPI slave module.

## Limitations

- only SPI2_HOST is used; a second simultaneous slave is not supported
- DMA mode is enabled automatically (`SPI_DMA_CH_AUTO`)
- `tx_buf` or `rx_buf` may be NULL for half-duplex operation, but not both

## Error Behavior

- invalid pins, unsupported mode, or null handle pointer return `BLUSYS_ERR_INVALID_ARG`
- timeout waiting for a master transfer returns `BLUSYS_ERR_TIMEOUT`
- driver initialization failures return the translated ESP-IDF error

## Example App

See `examples/spi_slave_basic/`.
