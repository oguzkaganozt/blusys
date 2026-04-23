# SPI

Blocking SPI for a master (single device per handle) and a synchronous slave. Master is fixed to SPI mode 0; slave supports all four modes.

## SPI Master

Blocking full-duplex master for single-device transfers. One handle owns one bus and one chip-select device.

### Quick Example

```c
blusys_spi_t *spi;
uint8_t tx_data[2] = {0xaa, 0x55};
uint8_t rx_data[2] = {0};

blusys_spi_open(0, 4, 6, 5, 7, 1000000, &spi);
blusys_spi_transfer(spi, tx_data, rx_data, sizeof(tx_data));
blusys_spi_close(spi);
```

### Common Mistakes

- choosing board pins reserved for flash, PSRAM, USB, or onboard peripherals
- forgetting the MOSI-to-MISO loopback wire during self-test
- expecting multi-device bus sharing from the current HAL (one handle → one device)

### Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

### Thread Safety

- concurrent transfers on the same handle are serialized internally
- different handles may be used independently when they use different SPI buses
- do not call `blusys_spi_close()` concurrently with other calls on the same handle

### Limitations

- SPI mode 0 is fixed
- one handle owns one bus and one chip-select line
- multi-device bus sharing and advanced queueing are not exposed

### Example App

See `examples/reference/hal` (SPI loopback scenario).

## SPI Slave

Synchronous slave-mode SPI: blocks with timeout waiting for the master to initiate a full-duplex exchange. DMA is enabled automatically. All four SPI modes supported.

### Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

### Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_spi_slave_close()` concurrently with other calls on the same handle

### ISR Notes

No ISR-safe calls are defined for the SPI slave module.

### Limitations

- only SPI2_HOST is used; a second simultaneous slave is not supported
- `tx_buf` or `rx_buf` may be NULL for half-duplex, but not both

### Example App

See `examples/validation/peripheral_lab/` (`periph_spi_slave` scenario).
