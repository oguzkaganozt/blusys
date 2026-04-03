# Scan An I2C Bus

## Problem Statement

You want to discover which 7-bit I2C device addresses acknowledge on an ESP32-C3 or ESP32-S3 board.

## Prerequisites

- ESP32-C3 or ESP32-S3 board
- SDA and SCL connected to the target I2C bus
- pull-up resistors on SDA and SCL, or a board that already provides them
- example I2C pins reviewed in `idf.py menuconfig` if your board uses different safe pins

## Minimal Example

```c
blusys_i2c_master_t *i2c;

blusys_i2c_master_open(0, 8, 9, 100000, &i2c);
blusys_i2c_master_probe(i2c, 0x3c, 20);
blusys_i2c_master_close(i2c);
```

## APIs Used

- `blusys_i2c_master_open()` configures the bus pins and frequency
- `blusys_i2c_master_probe()` checks whether a device address acknowledges
- `blusys_i2c_master_close()` releases the bus handle

## Expected Runtime Behavior

- the example scans addresses `0x08` through `0x77`
- acknowledged devices are printed one by one
- if the bus times out, the example prints a timeout message so wiring and pull-ups can be checked

## Common Mistakes

- forgetting external pull-ups on a bare bus
- scanning the wrong SDA and SCL pins for the board
- expecting a timeout to mean “no device”; on I2C it often points to a wiring or pull-up issue instead
