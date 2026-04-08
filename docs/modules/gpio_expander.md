# GPIO Expander

Unified driver for I2C and SPI GPIO expander ICs. Provides extra GPIO pins by communicating with an external chip over an existing bus. Supports PCF8574 / PCF8574A (8-bit, I2C) and MCP23017 (16-bit, I2C) / MCP23S17 (16-bit, SPI).

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [GPIO Expander Basics](../guides/gpio-expander-basic.md).

## Target Support

| Target   | Supported |
|----------|-----------|
| ESP32    | yes       |
| ESP32-C3 | yes       |
| ESP32-S3 | yes       |

Available on all three targets. The module depends only on I2C or SPI, both of which are base features.

## Types

### `blusys_gpio_expander_t`

```c
typedef struct blusys_gpio_expander blusys_gpio_expander_t;
```

Opaque handle returned by `blusys_gpio_expander_open()`.

### `blusys_gpio_expander_ic_t`

```c
typedef enum {
    BLUSYS_GPIO_EXPANDER_IC_PCF8574,    // 8-bit I2C, address 0x20–0x27
    BLUSYS_GPIO_EXPANDER_IC_PCF8574A,   // 8-bit I2C, address 0x38–0x3F
    BLUSYS_GPIO_EXPANDER_IC_MCP23017,   // 16-bit I2C, address 0x20–0x27
    BLUSYS_GPIO_EXPANDER_IC_MCP23S17,   // 16-bit SPI, hw_addr 0–7
} blusys_gpio_expander_ic_t;
```

Selects the physical IC. The IC type determines the transport (I2C vs SPI), register layout, pin count, and address encoding.

### `blusys_gpio_expander_dir_t`

```c
typedef enum {
    BLUSYS_GPIO_EXPANDER_OUTPUT = 0,
    BLUSYS_GPIO_EXPANDER_INPUT  = 1,
} blusys_gpio_expander_dir_t;
```

Pin direction, passed to `blusys_gpio_expander_set_direction()`.

### `blusys_gpio_expander_config_t`

```c
typedef struct {
    blusys_gpio_expander_ic_t  ic;
    blusys_i2c_master_t       *i2c;
    uint16_t                   i2c_address;
    blusys_spi_t              *spi;
    uint8_t                    hw_addr;
    int                        timeout_ms;
} blusys_gpio_expander_config_t;
```

| Field | Description |
|-------|-------------|
| `ic` | IC type |
| `i2c` | I2C bus handle (PCF8574/A, MCP23017); set to NULL for MCP23S17 |
| `i2c_address` | 7-bit I2C address |
| `spi` | SPI bus handle (MCP23S17); set to NULL for I2C ICs |
| `hw_addr` | MCP23S17 A2:A0 hardware address (0–7); ignored for I2C ICs |
| `timeout_ms` | Per-transaction timeout in ms; use `BLUSYS_TIMEOUT_FOREVER` to block |

The caller owns the bus handles and must keep them open for the lifetime of the expander handle.

## Functions

### `blusys_gpio_expander_open`

```c
blusys_err_t blusys_gpio_expander_open(const blusys_gpio_expander_config_t *config,
                                        blusys_gpio_expander_t **out_handle);
```

Opens a GPIO expander and performs IC-specific initialisation:

- **PCF8574/A** — writes `0xFF` to all pins to enable the weak quasi-bidirectional pull-up (all pins start as inputs).
- **MCP23017/S17** — writes `IOCON = 0x00` to force `BANK=0`, then reads back `IODIR` and `OLAT` into the internal cache.

| Parameter | Description |
|-----------|-------------|
| `config` | Expander configuration |
| `out_handle` | Written with the new handle on success |

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`, `BLUSYS_ERR_IO`, or `BLUSYS_ERR_TIMEOUT`.

---

### `blusys_gpio_expander_close`

```c
blusys_err_t blusys_gpio_expander_close(blusys_gpio_expander_t *handle);
```

Performs a best-effort hardware reset and frees the handle. Does **not** close the underlying I2C or SPI bus — that is the caller's responsibility.

Returns `BLUSYS_OK` or `BLUSYS_ERR_INVALID_ARG`.

---

### `blusys_gpio_expander_set_direction`

```c
blusys_err_t blusys_gpio_expander_set_direction(blusys_gpio_expander_t *handle,
                                                 uint8_t pin,
                                                 blusys_gpio_expander_dir_t dir);
```

Sets the direction of a single pin.

Pin numbering: 0–7 for 8-pin ICs. For 16-pin ICs: pins 0–7 map to port A, pins 8–15 to port B.

!!! note "PCF8574/A direction model"
    These ICs have no dedicated direction register. Setting a pin to `INPUT` sets the output latch bit to `1`, enabling the weak quasi-bidirectional pull-up. External circuitry can then pull the pin low to signal a logic 0.

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_IO`, or `BLUSYS_ERR_TIMEOUT`.

---

### `blusys_gpio_expander_write_pin`

```c
blusys_err_t blusys_gpio_expander_write_pin(blusys_gpio_expander_t *handle,
                                             uint8_t pin, bool level);
```

Sets the output level of a single pin and writes to hardware. For PCF8574/A, input-configured pins are automatically kept high in the output latch to preserve the weak pull-up.

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_IO`, or `BLUSYS_ERR_TIMEOUT`.

---

### `blusys_gpio_expander_read_pin`

```c
blusys_err_t blusys_gpio_expander_read_pin(blusys_gpio_expander_t *handle,
                                            uint8_t pin, bool *out_level);
```

Reads the current logic level of a single pin from hardware (not from the output latch cache).

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_IO`, or `BLUSYS_ERR_TIMEOUT`.

---

### `blusys_gpio_expander_write_port`

```c
blusys_err_t blusys_gpio_expander_write_port(blusys_gpio_expander_t *handle,
                                              uint16_t values);
```

Writes all output pins in a single bus transaction. `values` is a bitmask (bit N = pin N). Upper 8 bits are ignored for 8-pin ICs.

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_IO`, or `BLUSYS_ERR_TIMEOUT`.

---

### `blusys_gpio_expander_read_port`

```c
blusys_err_t blusys_gpio_expander_read_port(blusys_gpio_expander_t *handle,
                                             uint16_t *out_values);
```

Reads all pin states in a single bus transaction. `out_values` receives a bitmask (bit N = pin N). Upper 8 bits are always 0 for 8-pin ICs.

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_IO`, or `BLUSYS_ERR_TIMEOUT`.

---

## Lifecycle

1. Open an I2C or SPI bus with `blusys_i2c_master_open()` / `blusys_spi_open()`.
2. Call `blusys_gpio_expander_open()` with the bus handle and IC config.
3. Call `blusys_gpio_expander_set_direction()` for each pin.
4. Use `write_pin` / `read_pin` for individual pins or `write_port` / `read_port` for bulk operations.
5. Call `blusys_gpio_expander_close()`.
6. Close the bus with `blusys_i2c_master_close()` / `blusys_spi_close()`.

## Thread Safety

All public functions are thread-safe. An internal `blusys_lock_t` serialises concurrent access. The lock is held only for the duration of the bus transaction (microseconds to low milliseconds), not across blocking waits, so concurrent callers see short, bounded delays.

Multiple expanders can share one bus. Each expander has its own lock; the I2C/SPI module has its own internal lock. The lock hierarchy is always `expander_lock → bus_lock`, so there is no deadlock risk.

## Limitations

- **PCF8574/A** — no dedicated direction register; input pins rely on the quasi-bidirectional weak pull-up. Do not drive input-configured pins externally to logic high (the IC already does this); only pull them low.
- **MCP23S17** — the `timeout_ms` field is stored but not forwarded to SPI transactions, which are deterministic.
- **MCP23017/S17** — open always forces `IOCON.BANK=0`. Interrupt and pull-up registers are not exposed by this module.
- One expander instance per handle. Multiple instances at different I2C addresses or CS pins can coexist on the same bus.
