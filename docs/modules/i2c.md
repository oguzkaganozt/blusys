# I2C Master

## Purpose

The `i2c` module provides a blocking master-mode bus API that keeps the common path short:

- open an I2C master bus with SDA, SCL, and frequency
- probe a 7-bit device address
- write to a device
- read from a device
- perform write-then-read register transactions
- close the bus handle

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include <stdint.h>

#include "blusys/i2c.h"

void app_main(void)
{
    blusys_i2c_master_t *i2c;
    uint8_t who_am_i = 0;

    if (blusys_i2c_master_open(0, 8, 9, 100000, &i2c) != BLUSYS_OK) {
        return;
    }

    blusys_i2c_master_write_read(i2c, 0x68, "\x75", 1, &who_am_i, 1, 100);
    blusys_i2c_master_close(i2c);
}
```

## Lifecycle Model

I2C master is handle-based:

1. call `blusys_i2c_master_open()`
2. use probe, write, read, or write-read calls
3. call `blusys_i2c_master_close()` when finished

The public handle represents one configured bus.
Device selection stays per transfer through the `device_address` argument.

## Blocking APIs

- `blusys_i2c_master_open()`
- `blusys_i2c_master_close()`
- `blusys_i2c_master_probe()`
- `blusys_i2c_master_write()`
- `blusys_i2c_master_read()`
- `blusys_i2c_master_write_read()`

## Async APIs

None in Phase 3.

## Thread Safety

- concurrent operations on the same handle are serialized internally
- different I2C handles may be used independently when they use different hardware ports
- do not call `blusys_i2c_master_close()` concurrently with other calls using the same handle

## ISR Notes

Phase 3 does not define ISR-safe I2C calls.

## Limitations

- only 7-bit addressing is exposed in Phase 3
- only master mode is exposed
- internal pull-ups are enabled for convenience, but proper external pull-ups are still recommended
- the current handle keeps one backend device slot and switches its address between blocking transfers as needed

## Error Behavior

- invalid ports, pins, addresses, timeouts, or pointers return `BLUSYS_ERR_INVALID_ARG`
- bus-level timeout failures return `BLUSYS_ERR_TIMEOUT`
- missing devices and NACK-style probe failures are reported as `BLUSYS_ERR_IO`

## Example App

See `examples/i2c_scan/`.
