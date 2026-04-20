# I2C

Blocking I2C with a master and a slave peer. 7-bit addressing only.

Per-transfer device selection on the master side through the `device_address` argument; no per-device handle needed.

## I2C Master

### Quick Example

```c
blusys_i2c_master_t *i2c;

blusys_i2c_master_open(0, 8, 9, 100000, &i2c);
blusys_i2c_master_scan(i2c, 0x08, 0x77, 20, on_device, NULL, NULL);
blusys_i2c_master_close(i2c);
```

### Common Mistakes

- forgetting external pull-ups on a bare bus
- scanning the wrong SDA and SCL pins for the board
- expecting a timeout to mean "no device"; on I2C it often points to a wiring or pull-up issue instead

### Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

### Thread Safety

- concurrent operations on the same handle are serialized internally
- different handles may be used independently when they use different hardware ports
- do not call `blusys_i2c_master_close()` concurrently with other calls on the same handle

### Limitations

- only 7-bit addressing is supported
- only master mode is exposed
- the current implementation uses one backend device slot and switches its address between transfers as needed

### Example App

See `examples/reference/hal` (I2C scan scenario).

## I2C Slave

Receive bytes from a master and transmit bytes back on request.

### Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

### Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_i2c_slave_close()` concurrently with other calls on the same handle

### ISR Notes

The underlying receive completion is ISR-driven. The public `blusys_i2c_slave_receive()` API is task-safe and must not be called from an ISR context.

### Limitations

- only 7-bit addressing is supported
- port selection is automatic; no explicit port parameter
- uses the ESP-IDF V1 slave driver; the received byte count equals the `size` requested at `receive()` time
- `tx_buf_size` must be greater than zero

### Example App

See `examples/validation/peripheral_lab/` (`periph_i2c_slave` scenario).
