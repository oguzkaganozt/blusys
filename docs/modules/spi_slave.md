# SPI Slave

Synchronous slave-mode SPI: full-duplex exchange with a master, blocking with timeout.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [SPI Slave](../guides/spi-slave-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_spi_slave_t`

```c
typedef struct blusys_spi_slave blusys_spi_slave_t;
```

Opaque handle returned by `blusys_spi_slave_open()`.

## Functions

### `blusys_spi_slave_open`

```c
blusys_err_t blusys_spi_slave_open(int mosi_pin,
                                    int miso_pin,
                                    int clk_pin,
                                    int cs_pin,
                                    uint8_t mode,
                                    blusys_spi_slave_t **out_slave);
```

Initializes the SPI slave. DMA is enabled automatically.

**Parameters:**
- `mosi_pin` — GPIO for MOSI
- `miso_pin` — GPIO for MISO
- `clk_pin` — GPIO for clock
- `cs_pin` — GPIO for chip select
- `mode` — SPI clock mode (0–3); must match the master
- `out_slave` — output handle

**Clock modes:**

| `mode` | CPOL | CPHA | Clock idle / sample edge |
|--------|------|------|--------------------------|
| 0 | 0 | 0 | idle low / rising |
| 1 | 0 | 1 | idle low / falling |
| 2 | 1 | 0 | idle high / falling |
| 3 | 1 | 1 | idle high / rising |

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins or unsupported mode.

---

### `blusys_spi_slave_close`

```c
blusys_err_t blusys_spi_slave_close(blusys_spi_slave_t *slave);
```

Deinitializes the SPI slave and frees the handle.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `slave` is NULL.

---

### `blusys_spi_slave_transfer`

```c
blusys_err_t blusys_spi_slave_transfer(blusys_spi_slave_t *slave,
                                        const void *tx_buf,
                                        void *rx_buf,
                                        size_t size,
                                        int timeout_ms);
```

Blocks until the master initiates a transfer and exchanges `size` bytes. For half-duplex operation, one of `tx_buf` or `rx_buf` may be NULL, but not both.

**Parameters:**
- `slave` — handle
- `tx_buf` — data to send to master; may be NULL for receive-only
- `rx_buf` — buffer for data from master; may be NULL for send-only
- `size` — number of bytes to exchange
- `timeout_ms` — milliseconds to wait for master to initiate; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` if the master does not initiate before the timeout.

## Lifecycle

1. `blusys_spi_slave_open()` — configure slave
2. `blusys_spi_slave_transfer()` — wait for and exchange data with master
3. `blusys_spi_slave_close()` — release

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_spi_slave_close()` concurrently with other calls on the same handle

## ISR Notes

No ISR-safe calls are defined for the SPI slave module.

## Limitations

- only SPI2_HOST is used; a second simultaneous slave is not supported
- `tx_buf` or `rx_buf` may be NULL for half-duplex, but not both

## Example App

See `examples/spi_slave_basic/`.
