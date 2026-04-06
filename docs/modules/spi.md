# SPI Master

Blocking full-duplex SPI master for single-device transfers. Fixed to SPI mode 0.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [SPI Loopback](../guides/spi-loopback.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_spi_t`

```c
typedef struct blusys_spi blusys_spi_t;
```

Opaque handle returned by `blusys_spi_open()`. Owns both the SPI bus and one attached device.

## Functions

### `blusys_spi_open`

```c
blusys_err_t blusys_spi_open(int bus,
                             int sclk_pin,
                             int mosi_pin,
                             int miso_pin,
                             int cs_pin,
                             uint32_t freq_hz,
                             blusys_spi_t **out_spi);
```

Initializes an SPI bus and registers one device on it. SPI mode 0 is used.

**Parameters:**
- `bus` — SPI bus index (target-dependent; typically 0 or 1)
- `sclk_pin` — GPIO for clock
- `mosi_pin` — GPIO for MOSI
- `miso_pin` — GPIO for MISO; pass `-1` if not used
- `cs_pin` — GPIO for chip select
- `freq_hz` — bus clock frequency
- `out_spi` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments, `BLUSYS_ERR_INVALID_STATE` if the bus is already in use.

---

### `blusys_spi_close`

```c
blusys_err_t blusys_spi_close(blusys_spi_t *spi);
```

Removes the attached device, frees the bus, and releases the handle.

---

### `blusys_spi_transfer`

```c
blusys_err_t blusys_spi_transfer(blusys_spi_t *spi,
                                 const void *tx_data,
                                 void *rx_data,
                                 size_t size);
```

Performs a full-duplex transfer. `tx_data` and `rx_data` may point to the same buffer for in-place exchange. Pass NULL for `rx_data` if received data is not needed.

**Parameters:**
- `spi` — handle
- `tx_data` — data to send; must not be NULL
- `rx_data` — buffer for received data; may be NULL
- `size` — number of bytes to transfer

**Returns:** `BLUSYS_OK`, translated ESP-IDF error on transfer failure.

## Lifecycle

1. `blusys_spi_open()` — initialize bus and device
2. `blusys_spi_transfer()` — exchange bytes
3. `blusys_spi_close()` — release

## Thread Safety

- concurrent transfers on the same handle are serialized internally
- different handles may be used independently when they use different SPI buses
- do not call `blusys_spi_close()` concurrently with other calls on the same handle

## Limitations

- SPI mode 0 is fixed
- one handle owns one bus and one chip-select line
- multi-device bus sharing and advanced queueing are not exposed

## Example App

See `examples/spi_loopback/`.
