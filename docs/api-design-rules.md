# API Design Rules

## Core Rules

- all public symbols use the `blusys_` prefix
- public API is C only and must not expose ESP-IDF types
- names should be direct and readable
- keep the common path short and easy to call

## API Shape

- use `blusys_err_t` for public results
- use opaque handles for stateful peripherals
- use raw GPIO numbers publicly
- prefer simple scalar arguments for common operations
- use config structs only when the simple path is no longer enough

Typical shapes:

- `blusys_gpio_write(pin, level)`
- `blusys_uart_open(port, tx_pin, rx_pin, baudrate, &handle)`
- `blusys_i2c_master_open(port, sda_pin, scl_pin, freq_hz, &handle)`
- `blusys_spi_transfer(handle, tx_buf, rx_buf, len)`

## Error And Behavior Rules

- translate backend errors internally
- keep the public error model small
- document major failure cases
- treat blocking APIs as first-class
- add async only where it is naturally useful
- async should not require users to understand FreeRTOS internals

## Thread-Safety Rules

- define thread-safety per module
- protect lifecycle operations such as `close` against concurrent active use
- document ISR-safe behavior explicitly

## Stability Rules

- do not expose target-specific behavior unless all supported targets share it
- do not add public API before examples and docs exist
- prefer additive evolution over breaking redesign
