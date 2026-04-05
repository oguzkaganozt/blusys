# Changelog

## v1.0.0

First stable public release of Blusys HAL.

Included in `v1.0.0`:

- supported targets: `esp32`, `esp32c3`, `esp32s3`
- foundational modules: `version`, `error`, `target`, `system`
- digital IO: `gpio`, including interrupt callbacks
- communication: `uart`, `i2c`, `spi`, including UART async TX/RX callbacks
- timing and analog: `pwm`, `adc`, `timer`, including timer callbacks
- buildable examples for shipped modules
- simplified user documentation and module reference pages

Validation summary:

- full shipped example build matrix passed
- async hardware validation completed for `gpio_interrupt`, `uart_async`, and `timer_basic`
- communication example validation completed for `uart_loopback`, `i2c_scan`, and `spi_loopback`

Known scope limits for `v1.0.0`:

- the API is intentionally limited to the common surface across supported targets
- advanced chip-specific features remain out of scope for the core HAL
- formal unit-test infrastructure is still deferred; validation is build- and hardware-based
