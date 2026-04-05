# Receive And Echo Bytes As An I2C Slave

## Problem Statement

You want an ESP32 to act as an I2C slave device that receives bytes from a master and echoes them back.

## Prerequisites

- a supported board
- SDA and SCL connected to an I2C master (another ESP32, Raspberry Pi, logic analyzer, etc.)
- pull-up resistors on SDA and SCL, or a board that already provides them
- the slave address agreed upon with the master (default: `0x55`)

## Minimal Example

```c
blusys_i2c_slave_t *slave;
uint8_t buf[32];
size_t received;

blusys_i2c_slave_open(8, 9, 0x55, 128, &slave);
blusys_i2c_slave_receive(slave, buf, sizeof(buf), &received, 5000);
blusys_i2c_slave_transmit(slave, buf, received, 5000);
blusys_i2c_slave_close(slave);
```

## APIs Used

- `blusys_i2c_slave_open()` configures the SDA/SCL pins, 7-bit slave address, and TX buffer size
- `blusys_i2c_slave_receive()` blocks until the master sends data or the timeout expires
- `blusys_i2c_slave_transmit()` queues bytes for the master to read on the next read transaction
- `blusys_i2c_slave_close()` releases the handle

## Expected Runtime Behavior

- the slave waits up to the configured timeout for the master to write
- on receive, the example prints the received bytes and echoes them back
- if the timeout expires with no data, it continues to the next round

## Common Mistakes

- forgetting external pull-ups on a bare bus
- mismatched slave address between master and slave
- calling `receive()` and expecting it to return partial data — the V1 driver reports the full requested size as received even if fewer bytes arrived

## Example App

See `examples/i2c_slave_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.
