# I2C Slave

## Purpose

The `i2c_slave` module provides a callback-driven slave-mode I2C API:

- open an I2C slave device with SDA, SCL, 7-bit address, and TX buffer size
- wait to receive bytes from a master (blocking with timeout)
- transmit bytes to a master on request (blocking with timeout)
- close the slave handle

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include <stdint.h>

#include "blusys/i2c_slave.h"

void app_main(void)
{
    blusys_i2c_slave_t *slave;
    uint8_t buf[32];
    size_t received;

    if (blusys_i2c_slave_open(8, 9, 0x55, 128, &slave) != BLUSYS_OK) {
        return;
    }

    blusys_i2c_slave_receive(slave, buf, sizeof(buf), &received, 5000);
    blusys_i2c_slave_transmit(slave, buf, received, 5000);
    blusys_i2c_slave_close(slave);
}
```

## Lifecycle Model

I2C slave is handle-based:

1. call `blusys_i2c_slave_open()` — configures pins, address, and TX buffer
2. call `blusys_i2c_slave_receive()` to block until a master writes to the device
3. call `blusys_i2c_slave_transmit()` to queue bytes for the next master read
4. call `blusys_i2c_slave_close()` when finished

## Blocking APIs

- `blusys_i2c_slave_open()`
- `blusys_i2c_slave_close()`
- `blusys_i2c_slave_receive()` — blocks until data arrives or timeout expires
- `blusys_i2c_slave_transmit()` — blocks until bytes are queued or timeout expires

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_i2c_slave_close()` concurrently with other calls using the same handle

## ISR Notes

The underlying receive completion is ISR-driven. The public `blusys_i2c_slave_receive()` API is task-safe and must not be called from an ISR context.

## Limitations

- only 7-bit addressing is supported
- port selection is automatic; there is no explicit port parameter
- uses the ESP-IDF V1 slave driver; the received byte count equals the byte count requested at `receive()` time (the V1 driver does not report a partial count)
- `tx_buf_size` must be greater than zero

## Error Behavior

- invalid pins, zero buffer size, or null pointers return `BLUSYS_ERR_INVALID_ARG`
- timeouts waiting for master activity return `BLUSYS_ERR_TIMEOUT`
- driver initialization failures return the translated ESP-IDF error

## Example App

See `examples/i2c_slave_basic/`.
