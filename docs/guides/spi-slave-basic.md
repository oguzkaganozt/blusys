# Exchange Bytes As An SPI Slave

## Problem Statement

You want an ESP32 to act as an SPI slave device that receives and transmits bytes simultaneously with an SPI master.

## Prerequisites

- a supported board
- MOSI, MISO, CLK, and CS wired to an SPI master (another ESP32, microcontroller, or SPI analyzer)
- the clock mode (CPOL/CPHA) agreed upon with the master (default: mode 0)

## Minimal Example

```c
blusys_spi_slave_t *slave;
uint8_t rx[16];
uint8_t tx[16];

memset(tx, 0xAB, sizeof(tx));
blusys_spi_slave_open(11, 13, 12, 10, 0, &slave);
blusys_spi_slave_transfer(slave, tx, rx, sizeof(rx), 5000);
blusys_spi_slave_close(slave);
```

## APIs Used

- `blusys_spi_slave_open()` configures MOSI, MISO, CLK, CS pins and SPI clock mode
- `blusys_spi_slave_transfer()` blocks until the master drives a transfer, then exchanges bytes full-duplex
- `blusys_spi_slave_close()` releases the handle

## Expected Runtime Behavior

- the slave loads the TX buffer and waits for the master to initiate a transaction
- bytes are clocked simultaneously in both directions
- after the transfer, RX contains what the master sent and the master received what was in TX

## Common Mistakes

- clock mode mismatch between master and slave; both must use the same CPOL/CPHA combination
- forgetting that SPI is always full-duplex: the slave must prepare TX before the master clocks

## Example App

See `examples/spi_slave_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.


## API Reference

For full type definitions and function signatures, see [SPI Slave API Reference](../modules/spi_slave.md).
