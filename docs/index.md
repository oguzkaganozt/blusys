# Blusys HAL

Blusys HAL is a simplified C hardware abstraction layer for ESP32 devices using ESP-IDF v5.5.

## What It Gives You

- one common API across `ESP32`, `ESP32-C3`, and `ESP32-S3`
- a smaller public API than raw ESP-IDF for common tasks
- buildable examples for every shipped module
- short task guides plus module reference pages

## Start Here

1. `guides/getting-started.md`
2. pick a task guide from `guides/`
3. use the matching module reference in `modules/` when you need API details

## Modules

- `system`
- `gpio`
- `uart`
- `i2c`
- `spi`
- `pwm`
- `adc`
- `timer`
- `pcnt`
- `rmt`

## Supported Targets

- `esp32`
- `esp32c3`
- `esp32s3`

## Project Status

`v1.0.0` is released.
`V2` is in progress with `pcnt` and `rmt`.

## Need More Detail?

- Compatibility and target notes: `target-matrix.md`
- Hardware validation guide: `guides/hardware-smoke-tests.md`
- Release notes: `../CHANGELOG.md`
- Contributor guidance: `contributing.md`
