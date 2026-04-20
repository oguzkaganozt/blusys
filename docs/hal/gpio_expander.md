# GPIO Expander

Unified driver for I2C and SPI GPIO expander ICs. Adds extra GPIO pins over an existing bus. Supports PCF8574 / PCF8574A (8-bit, I2C) and MCP23017 (16-bit, I2C) / MCP23S17 (16-bit, SPI).

## Quick Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    /* 1. Open the I2C bus */
    blusys_i2c_master_t *i2c;
    blusys_i2c_master_open(0, /*sda=*/21, /*scl=*/22, 400000, &i2c);

    /* 2. Open the expander */
    blusys_gpio_expander_t *exp;
    blusys_gpio_expander_config_t cfg = {
        .ic          = BLUSYS_GPIO_EXPANDER_IC_PCF8574,
        .i2c         = i2c,
        .i2c_address = 0x20,
        .timeout_ms  = 100,
    };
    blusys_gpio_expander_open(&cfg, &exp);

    /* 3. Configure directions and I/O */
    blusys_gpio_expander_set_direction(exp, 0, BLUSYS_GPIO_EXPANDER_OUTPUT);
    blusys_gpio_expander_set_direction(exp, 7, BLUSYS_GPIO_EXPANDER_INPUT);
    blusys_gpio_expander_write_pin(exp, 0, true);

    bool level;
    blusys_gpio_expander_read_pin(exp, 7, &level);

    /* 4. Cleanup — close the expander first, then the bus */
    blusys_gpio_expander_close(exp);
    blusys_i2c_master_close(i2c);
}
```

## Common Mistakes

1. **Wrong address range.** PCF8574 uses 0x20–0x27; PCF8574A uses 0x38–0x3F. Mixing them returns `BLUSYS_ERR_INVALID_ARG` at open time.
2. **Closing the bus before the expander.** Always call `gpio_expander_close()` before `i2c_master_close()` / `spi_close()` — the expander's close function still accesses the bus to reset hardware.
3. **Forgetting the I2C pull-ups.** The ESP32 internal pull-ups (~45 kΩ) are often too weak at 400 kHz; external 4.7 kΩ resistors are recommended.
4. **Writing to an input pin on PCF8574.** The write succeeds, but the module keeps input-pin latch bits at 1 to preserve the weak pull-up. Writing `false` to an input pin has no visible effect.
5. **Forgetting `set_direction()`.** MCP23017/S17 resets to all-inputs; drive pins only after `set_direction(pin, OUTPUT)`.

## Target Support

| Target   | Supported |
|----------|-----------|
| ESP32    | yes       |
| ESP32-C3 | yes       |
| ESP32-S3 | yes       |

The module depends only on I2C or SPI, both of which are base features.

## Thread Safety

- all public functions are thread-safe; an internal lock serialises concurrent access
- the lock is held only for the duration of the bus transaction (microseconds to low milliseconds)
- multiple expanders can share one bus; the lock hierarchy is always `expander_lock → bus_lock`, so there is no deadlock risk

## ISR Notes

No ISR-safe calls are defined for the GPIO expander module.

## Limitations

- **PCF8574/A** — no dedicated direction register; input pins rely on the quasi-bidirectional weak pull-up. Do not drive input-configured pins to logic high externally — only pull them low.
- **MCP23S17** — `timeout_ms` is stored but not forwarded to SPI transactions, which are deterministic.
- **MCP23017/S17** — open always forces `IOCON.BANK=0`; interrupt and pull-up registers are not exposed.
- one expander instance per handle; multiple instances at different addresses or CS pins can coexist on the same bus.

## Example App

See `examples/reference/hal` (gpio_expander scenario).
