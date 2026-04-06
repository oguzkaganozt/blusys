# I2C Slave

Slave-mode I2C: receive bytes from a master and transmit bytes back on request.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [I2C Slave](../guides/i2c-slave-basic.md).

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

See `examples/i2c_slave_basic/`.
