# API Design Rules

## Naming

- all public symbols use the `blusys_` prefix
- names should be direct and readable
- prefer balanced names such as `blusys_gpio_write` over overly long names
- avoid names that mirror ESP-IDF unnecessarily when a simpler name is clearer

## Public API Language

- public API is C only
- public headers must be C++ compatible through `extern "C"` guards when implementation starts
- no public dependence on ESP-IDF types

## Type Rules

- use `blusys_err_t` for public function results
- use opaque handles for stateful peripherals
- use raw GPIO numbers publicly
- allow simple scalar arguments for common operations
- use small config structs only for advanced or extensible setup paths

## Simplicity Rules

- common path must be easy to call
- avoid forcing users to build complex config objects for simple tasks
- avoid hidden background tasks in v1
- add backend power-user options only when there is a clear need

## Function Shape Rules

Preferred function patterns:
- `blusys_gpio_set_output(pin)`
- `blusys_gpio_write(pin, level)`
- `blusys_uart_open(port, tx_pin, rx_pin, baudrate, &handle)`
- `blusys_i2c_master_open(port, sda_pin, scl_pin, freq_hz, &handle)`
- `blusys_spi_transfer(handle, tx_buf, rx_buf, len)`

Advanced setup can use `_ex` forms later if needed:
- `blusys_uart_open_ex(...)`
- `blusys_spi_open_ex(...)`

## Error Model

The public API uses a custom compact error model.

Recommended base enum:

```c
typedef enum {
    BLUSYS_OK = 0,
    BLUSYS_ERR_INVALID_ARG,
    BLUSYS_ERR_INVALID_STATE,
    BLUSYS_ERR_NOT_SUPPORTED,
    BLUSYS_ERR_TIMEOUT,
    BLUSYS_ERR_BUSY,
    BLUSYS_ERR_NO_MEM,
    BLUSYS_ERR_IO,
    BLUSYS_ERR_INTERNAL,
} blusys_err_t;
```

Rules:
- translate all backend errors internally
- keep the public error set small
- document every function's major failure cases

## Blocking And Async Rules

- blocking APIs are first-class
- async support must exist where it is naturally useful
- async APIs should be callback-based or event-based
- async must not require users to understand FreeRTOS internals

Minimum async support for v1:
- GPIO interrupts and callbacks
- UART async TX and RX
- timer callbacks

## Thread-Safety Rules

- design thread safety from the beginning
- document thread-safety guarantees per module
- each handle may own an internal lock if needed
- functions safe in ISR context must be explicitly documented
- lifecycle functions such as `close` must be protected against concurrent active use

## Stability Rules

- do not expose target-specific behavior in common APIs unless all supported targets share it
- do not add features to public headers before docs and examples exist
- prefer additive evolution over breaking redesign
