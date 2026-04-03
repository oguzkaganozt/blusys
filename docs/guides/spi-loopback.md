# SPI Loopback

## Problem Statement

You want to confirm that a blocking SPI transfer can round-trip bytes through the Blusys Phase 3 SPI API.

## Prerequisites

- ESP32-C3 or ESP32-S3 board
- a jumper wire from the example MOSI pin to the example MISO pin
- example pins reviewed in `idf.py menuconfig` if your board uses different safe pins

## Minimal Example

```c
blusys_spi_t *spi;
uint8_t tx_data[2] = {0xaa, 0x55};
uint8_t rx_data[2] = {0};

blusys_spi_open(0, 4, 6, 5, 7, 1000000, &spi);
blusys_spi_transfer(spi, tx_data, rx_data, sizeof(tx_data));
blusys_spi_close(spi);
```

## APIs Used

- `blusys_spi_open()` configures one SPI bus and one attached device slot
- `blusys_spi_transfer()` performs one blocking full-duplex transfer
- `blusys_spi_close()` releases the SPI resources

## Expected Runtime Behavior

- the example prints the configured bus and pins
- with MOSI connected to MISO, the received bytes should match the transmitted bytes
- if the bus is already in use or the pins are invalid for the board, the example reports the translated Blusys error

## Common Mistakes

- choosing board pins that are reserved for flash, PSRAM, USB, or onboard peripherals
- forgetting the MOSI-to-MISO loopback wire
- assuming Phase 3 supports advanced multi-device SPI sharing; the first implementation intentionally keeps one handle tied to one bus and one device
