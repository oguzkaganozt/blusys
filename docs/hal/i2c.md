# I2C Master

Blocking master-mode I2C bus: probe, write, read, and write-then-read register transactions.

## Quick Example

```c
blusys_i2c_master_t *i2c;

blusys_i2c_master_open(0, 8, 9, 100000, &i2c);
blusys_i2c_master_scan(i2c, 0x08, 0x77, 20, on_device, NULL, NULL);
blusys_i2c_master_close(i2c);
```

## Common Mistakes

- forgetting external pull-ups on a bare bus
- scanning the wrong SDA and SCL pins for the board
- expecting a timeout to mean “no device”; on I2C it often points to a wiring or pull-up issue instead

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_i2c_master_t`

```c
typedef struct blusys_i2c_master blusys_i2c_master_t;
```

Opaque handle returned by `blusys_i2c_master_open()`. Represents one configured I2C bus.

### `blusys_i2c_scan_callback_t`

```c
typedef bool (*blusys_i2c_scan_callback_t)(uint16_t device_address, void *user_ctx);
```

Called for each address that responds during a scan. Return `false` to continue scanning, `true` to stop early.

## Functions

### `blusys_i2c_master_open`

```c
blusys_err_t blusys_i2c_master_open(int port,
                                    int sda_pin,
                                    int scl_pin,
                                    uint32_t freq_hz,
                                    blusys_i2c_master_t **out_i2c);
```

Opens an I2C master bus. Internal pull-ups are enabled; proper external pull-ups are still recommended.

**Parameters:**
- `port` — I2C port number (0 or 1)
- `sda_pin` — GPIO for SDA
- `scl_pin` — GPIO for SCL
- `freq_hz` — bus clock frequency (e.g. 100000 or 400000)
- `out_i2c` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.

---

### `blusys_i2c_master_close`

```c
blusys_err_t blusys_i2c_master_close(blusys_i2c_master_t *i2c);
```

Releases the I2C bus and frees the handle.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `i2c` is NULL.

---

### `blusys_i2c_master_probe`

```c
blusys_err_t blusys_i2c_master_probe(blusys_i2c_master_t *i2c, uint16_t device_address, int timeout_ms);
```

Checks whether a device responds at the given address.

**Parameters:**
- `i2c` — handle
- `device_address` — 7-bit device address
- `timeout_ms` — milliseconds to wait

**Returns:** `BLUSYS_OK` if the device ACKs, `BLUSYS_ERR_IO` if no ACK (device absent), `BLUSYS_ERR_TIMEOUT` on bus timeout.

---

### `blusys_i2c_master_scan`

```c
blusys_err_t blusys_i2c_master_scan(blusys_i2c_master_t *i2c,
                                    uint16_t start_address,
                                    uint16_t end_address,
                                    int timeout_ms,
                                    blusys_i2c_scan_callback_t callback,
                                    void *user_ctx,
                                    size_t *out_count);
```

Probes each address in `[start_address, end_address]` and calls `callback` for each that responds.

**Parameters:**
- `start_address` / `end_address` — address range (7-bit)
- `timeout_ms` — per-address probe timeout
- `callback` — called for each responding device; may be NULL
- `out_count` — total number of responding devices found; may be NULL

**Returns:** `BLUSYS_OK` on completion.

---

### `blusys_i2c_master_write`

```c
blusys_err_t blusys_i2c_master_write(blusys_i2c_master_t *i2c,
                                     uint16_t device_address,
                                     const void *data,
                                     size_t size,
                                     int timeout_ms);
```

Sends bytes to a device.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` on bus timeout, `BLUSYS_ERR_IO` on NACK.

---

### `blusys_i2c_master_read`

```c
blusys_err_t blusys_i2c_master_read(blusys_i2c_master_t *i2c,
                                    uint16_t device_address,
                                    void *data,
                                    size_t size,
                                    int timeout_ms);
```

Reads bytes from a device.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` on bus timeout, `BLUSYS_ERR_IO` on NACK.

---

### `blusys_i2c_master_write_read`

```c
blusys_err_t blusys_i2c_master_write_read(blusys_i2c_master_t *i2c,
                                          uint16_t device_address,
                                          const void *tx_data,
                                          size_t tx_size,
                                          void *rx_data,
                                          size_t rx_size,
                                          int timeout_ms);
```

Performs a write followed by a repeated-start read in one atomic transaction. Typical use: write a register address, read the register value.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`, `BLUSYS_ERR_IO` on NACK.

## Lifecycle

1. `blusys_i2c_master_open()` — configure bus
2. `blusys_i2c_master_probe()` / `blusys_i2c_master_write()` / `blusys_i2c_master_read()` / `blusys_i2c_master_write_read()` — transfers
3. `blusys_i2c_master_close()` — release

Device selection is per-transfer through the `device_address` argument; no per-device handle is needed.

## Thread Safety

- concurrent operations on the same handle are serialized internally
- different handles may be used independently when they use different hardware ports
- do not call `blusys_i2c_master_close()` concurrently with other calls on the same handle

## Limitations

- only 7-bit addressing is supported
- only master mode is exposed
- the current implementation uses one backend device slot and switches its address between transfers as needed

## Example App

See `examples/reference/hal` (I2C scan scenario).

## Slave Mode

Slave-mode I2C: receive bytes from a master and transmit bytes back on request.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_i2c_slave_t`

```c
typedef struct blusys_i2c_slave blusys_i2c_slave_t;
```

Opaque handle returned by `blusys_i2c_slave_open()`.

## Functions

### `blusys_i2c_slave_open`

```c
blusys_err_t blusys_i2c_slave_open(int sda_pin,
                                    int scl_pin,
                                    uint16_t addr,
                                    uint32_t tx_buf_size,
                                    blusys_i2c_slave_t **out_slave);
```

Opens an I2C slave device. Port selection is automatic.

**Parameters:**
- `sda_pin` — GPIO for SDA
- `scl_pin` — GPIO for SCL
- `addr` — 7-bit slave address
- `tx_buf_size` — size of the internal TX buffer in bytes; must be greater than zero
- `out_slave` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins, zero buffer size, or NULL pointer.

---

### `blusys_i2c_slave_close`

```c
blusys_err_t blusys_i2c_slave_close(blusys_i2c_slave_t *slave);
```

Releases the I2C slave and frees the handle.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `slave` is NULL.

---

### `blusys_i2c_slave_receive`

```c
blusys_err_t blusys_i2c_slave_receive(blusys_i2c_slave_t *slave,
                                       uint8_t *buf,
                                       size_t size,
                                       size_t *bytes_received,
                                       int timeout_ms);
```

Blocks until `size` bytes are received from the master or the timeout expires.

**Parameters:**
- `slave` — handle
- `buf` — receive buffer
- `size` — number of bytes to wait for
- `bytes_received` — actual bytes received
- `timeout_ms` — milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` if data does not arrive in time.

---

### `blusys_i2c_slave_transmit`

```c
blusys_err_t blusys_i2c_slave_transmit(blusys_i2c_slave_t *slave,
                                        const uint8_t *buf,
                                        size_t size,
                                        int timeout_ms);
```

Queues bytes into the TX buffer for the master to read on the next read transaction. Blocks until the bytes are queued or the timeout expires.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` on queue failure.

## Lifecycle

1. `blusys_i2c_slave_open()` — configure slave address and buffers
2. `blusys_i2c_slave_receive()` — block for master writes
3. `blusys_i2c_slave_transmit()` — queue bytes for master reads
4. `blusys_i2c_slave_close()` — release

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_i2c_slave_close()` concurrently with other calls on the same handle

## ISR Notes

The underlying receive completion is ISR-driven. The public `blusys_i2c_slave_receive()` API is task-safe and must not be called from an ISR context.

## Limitations

- only 7-bit addressing is supported
- port selection is automatic; no explicit port parameter
- uses the ESP-IDF V1 slave driver; the received byte count equals the `size` requested at `receive()` time
- `tx_buf_size` must be greater than zero

## Example App

See `examples/validation/i2c_slave_basic/`.
