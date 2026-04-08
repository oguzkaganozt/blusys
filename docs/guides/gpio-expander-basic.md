# Expand GPIO Pins with an I2C/SPI Expander

## Problem Statement

Your project needs more GPIO pins than the ESP32 provides, or you want to control many LEDs, relays, or buttons without consuming all available pins. An external GPIO expander IC connected over I2C or SPI gives you 8 or 16 additional pins with a simple read/write API.

## Prerequisites

- Any supported board (ESP32, ESP32-C3, or ESP32-S3)
- A PCF8574, PCF8574A, MCP23017, or MCP23S17 GPIO expander IC
- For I2C ICs: two GPIO pins for SDA and SCL (4.7 kΩ pull-ups to 3.3 V recommended)
- For MCP23S17: four GPIO pins for SCLK, MOSI, MISO, and CS

**Wiring (PCF8574, I2C):**

```
ESP32 SDA ──── PCF8574 SDA
ESP32 SCL ──── PCF8574 SCL
3.3 V ─┬──── PCF8574 VCC
       ├── 4.7 kΩ ── SDA
       └── 4.7 kΩ ── SCL
GND ──────── PCF8574 GND
A0/A1/A2 ── GND (address = 0x20) or VCC to set bits
```

## Minimal Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    // 1. Open the I2C bus
    blusys_i2c_master_t *i2c;
    blusys_i2c_master_open(0, /*sda=*/21, /*scl=*/22, 400000, &i2c);

    // 2. Open the expander
    blusys_gpio_expander_t *exp;
    blusys_gpio_expander_config_t cfg = {
        .ic          = BLUSYS_GPIO_EXPANDER_IC_PCF8574,
        .i2c         = i2c,
        .i2c_address = 0x20,
        .timeout_ms  = 100,
    };
    blusys_gpio_expander_open(&cfg, &exp);

    // 3. Configure directions
    blusys_gpio_expander_set_direction(exp, 0, BLUSYS_GPIO_EXPANDER_OUTPUT);
    blusys_gpio_expander_set_direction(exp, 7, BLUSYS_GPIO_EXPANDER_INPUT);

    // 4. Write and read
    blusys_gpio_expander_write_pin(exp, 0, true);   // pin 0 → high
    bool level;
    blusys_gpio_expander_read_pin(exp, 7, &level);  // read pin 7

    // 5. Bulk port access
    uint16_t all_pins;
    blusys_gpio_expander_read_port(exp, &all_pins);

    // 6. Cleanup
    blusys_gpio_expander_close(exp);
    blusys_i2c_master_close(i2c);
}
```

## Using a 16-bit Expander (MCP23017)

The API is identical — only the config changes:

```c
blusys_gpio_expander_config_t cfg = {
    .ic          = BLUSYS_GPIO_EXPANDER_IC_MCP23017,
    .i2c         = i2c,
    .i2c_address = 0x20,
    .timeout_ms  = 100,
};
blusys_gpio_expander_open(&cfg, &exp);

// Pins 0–7 → port A, pins 8–15 → port B
blusys_gpio_expander_set_direction(exp, 8, BLUSYS_GPIO_EXPANDER_OUTPUT);
blusys_gpio_expander_write_pin(exp, 8, true);   // port B pin 0
```

## Using the SPI Variant (MCP23S17)

```c
blusys_spi_t *spi;
blusys_spi_open(1, /*sclk=*/18, /*mosi=*/23, /*miso=*/19, /*cs=*/5, 1000000, &spi);

blusys_gpio_expander_config_t cfg = {
    .ic       = BLUSYS_GPIO_EXPANDER_IC_MCP23S17,
    .spi      = spi,
    .hw_addr  = 0,   // A0/A1/A2 all tied to GND
    .timeout_ms = 0, // ignored for SPI
};
blusys_gpio_expander_open(&cfg, &exp);
```

## Sharing a Bus Between Multiple Expanders

Multiple expanders at different addresses can share one bus handle:

```c
blusys_i2c_master_t *i2c;
blusys_i2c_master_open(0, 21, 22, 400000, &i2c);

blusys_gpio_expander_t *exp_a, *exp_b;
blusys_gpio_expander_config_t cfg_a = { .i2c = i2c, .i2c_address = 0x20, /* ... */ };
blusys_gpio_expander_config_t cfg_b = { .i2c = i2c, .i2c_address = 0x21, /* ... */ };
blusys_gpio_expander_open(&cfg_a, &exp_a);
blusys_gpio_expander_open(&cfg_b, &exp_b);

// Both share the same bus; internal locking handles concurrent access.

blusys_gpio_expander_close(exp_b);
blusys_gpio_expander_close(exp_a);
blusys_i2c_master_close(i2c);  // close bus last
```

## APIs Used

- `blusys_i2c_master_open()` / `blusys_i2c_master_close()`
- `blusys_spi_open()` / `blusys_spi_close()` (MCP23S17 only)
- `blusys_gpio_expander_open()` / `blusys_gpio_expander_close()`
- `blusys_gpio_expander_set_direction()`
- `blusys_gpio_expander_write_pin()` / `blusys_gpio_expander_read_pin()`
- `blusys_gpio_expander_write_port()` / `blusys_gpio_expander_read_port()`

## Expected Runtime Behavior

```
GPIO Expander Basic | target: esp32s3 | IC: PCF8574
I2C bus opened (SDA=8, SCL=9, addr=0x20)
GPIO expander opened (8 pins)
Pins 0–3: OUTPUT | Pins 4–7: INPUT
[ 0] write=0x0001  read_port=0x00F1  pins: P0=1 P1=0 P2=0 P3=0 P4=1 P5=1 P6=1 P7=1
[ 1] write=0x0002  read_port=0x00F2  pins: P0=0 P1=1 P2=0 P3=0 P4=1 P5=1 P6=1 P7=1
...
Expander closed
I2C bus closed
Done
```

Input pins (4–7) read back as 1 because no external signal is pulling them low.

## Common Mistakes

1. **Wrong address range.** PCF8574 uses 0x20–0x27; PCF8574A uses 0x38–0x3F. Mixing them returns `BLUSYS_ERR_INVALID_ARG` at open time.
2. **Closing the bus before the expander.** Always call `gpio_expander_close()` before `i2c_master_close()` / `spi_close()`. The expander's close function still accesses the bus to reset hardware.
3. **Forgetting the I2C pull-ups.** The ESP32 internal pull-ups (~45 kΩ) are often too weak at 400 kHz. External 4.7 kΩ resistors are recommended.
4. **Writing to an input pin on PCF8574.** The write succeeds (the latch bit is set), but the module automatically keeps input-pin latch bits at 1 to preserve the weak pull-up. Writing `false` to an input pin has no visible effect.
5. **Forgetting `set_direction()`.** MCP23017/S17 resets to all-inputs (`IODIR=0xFF`). If you want to drive outputs you must call `set_direction(pin, OUTPUT)` first.

## Example App

See `examples/gpio_expander_basic/` for a complete project that supports all four IC types via menuconfig, demonstrates direction setup, rotating output patterns, and port reads on all targets.

## API Reference

[GPIO Expander API](../modules/gpio_expander.md)
