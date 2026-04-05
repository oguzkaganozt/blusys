# Scan An I2C Bus

## Problem Statement

You want to discover which 7-bit I2C device addresses acknowledge on a supported board.

## Prerequisites

- a supported board
- SDA and SCL connected to the target I2C bus
- pull-up resistors on SDA and SCL, or a board that already provides them
- example I2C pins reviewed in `idf.py menuconfig` if your board uses different safe pins

## Minimal Example

```c
blusys_i2c_master_t *i2c;

blusys_i2c_master_open(0, 8, 9, 100000, &i2c);
blusys_i2c_master_scan(i2c, 0x08, 0x77, 20, on_device, NULL, NULL);
blusys_i2c_master_close(i2c);
```

## APIs Used

- `blusys_i2c_master_open()` configures the bus pins and frequency
- `blusys_i2c_master_scan()` scans a normal 7-bit address range and reports acknowledged devices through a callback while preserving ESP-IDF's `not found` vs timeout behavior
- `blusys_i2c_master_close()` releases the bus handle

## Expected Runtime Behavior

- the example scans addresses `0x08` through `0x77`
- acknowledged devices are printed one by one
- if the bus times out, the example prints a scan error so wiring and pull-ups can be checked

## Common Mistakes

- forgetting external pull-ups on a bare bus
- scanning the wrong SDA and SCL pins for the board
- expecting a timeout to mean “no device”; on I2C it often points to a wiring or pull-up issue instead

## Example App

See `examples/i2c_scan/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.
