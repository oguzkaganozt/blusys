# SPI Master

Blocking full-duplex SPI master for single-device transfers. Fixed to SPI mode 0.

## Quick Example

```c
blusys_spi_t *spi;
uint8_t tx_data[2] = {0xaa, 0x55};
uint8_t rx_data[2] = {0};

blusys_spi_open(0, 4, 6, 5, 7, 1000000, &spi);
blusys_spi_transfer(spi, tx_data, rx_data, sizeof(tx_data));
blusys_spi_close(spi);
```

## Common Mistakes

- choosing board pins that are reserved for flash, PSRAM, USB, or onboard peripherals
- forgetting the MOSI-to-MISO loopback wire
- assuming Phase 3 supports advanced multi-device SPI sharing; the first implementation intentionally keeps one handle tied to one bus and one device

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

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `spi` is NULL.

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

See `examples/reference/spi_loopback/`.

## Slave Mode

Synchronous slave-mode SPI: full-duplex exchange with a master, blocking with timeout.

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

See `examples/validation/spi_slave_basic/`.
