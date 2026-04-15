#if CONFIG_PERIPHERAL_LAB_SCENARIO_GPIO_EXPANDER
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "blusys/blusys.h"

/* ── Determine IC type at compile time ───────────────────────────────────── */

#if defined(CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_IC_PCF8574)
#  define EXAMPLE_IC      BLUSYS_GPIO_EXPANDER_IC_PCF8574
#  define EXAMPLE_IC_NAME "PCF8574"
#  define EXAMPLE_PINS    8
#elif defined(CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_IC_PCF8574A)
#  define EXAMPLE_IC      BLUSYS_GPIO_EXPANDER_IC_PCF8574A
#  define EXAMPLE_IC_NAME "PCF8574A"
#  define EXAMPLE_PINS    8
#elif defined(CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_IC_MCP23017)
#  define EXAMPLE_IC      BLUSYS_GPIO_EXPANDER_IC_MCP23017
#  define EXAMPLE_IC_NAME "MCP23017"
#  define EXAMPLE_PINS    16
#elif defined(CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_IC_MCP23S17)
#  define EXAMPLE_IC      BLUSYS_GPIO_EXPANDER_IC_MCP23S17
#  define EXAMPLE_IC_NAME "MCP23S17"
#  define EXAMPLE_PINS    16
#else
#  error "No IC type selected in menuconfig"
#endif

#define EXAMPLE_TIMEOUT_MS 100

void run_periph_gpio_expander(void)
{
    printf("GPIO Expander Basic | target: %s | IC: %s\n",
           blusys_target_name(), EXAMPLE_IC_NAME);

    blusys_err_t err;

#if defined(CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_IC_MCP23S17)

    /* ── SPI path ─────────────────────────────────────────────────────────── */
    blusys_spi_t *spi = NULL;
    err = blusys_spi_open(
        1,                                              /* SPI2 host */
        CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_SPI_SCLK,
        CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_SPI_MOSI,
        CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_SPI_MISO,
        CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_SPI_CS,
        1000000u,                                       /* 1 MHz */
        &spi);
    if (err != BLUSYS_OK) {
        printf("spi_open failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("SPI bus opened\n");

    blusys_gpio_expander_config_t config = {
        .ic       = EXAMPLE_IC,
        .spi      = spi,
        .hw_addr  = CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_SPI_HW_ADDR,
        .timeout_ms = EXAMPLE_TIMEOUT_MS,
    };

#else

    /* ── I2C path ─────────────────────────────────────────────────────────── */
    blusys_i2c_master_t *i2c = NULL;
    err = blusys_i2c_master_open(
        0,                                              /* I2C port 0 */
        CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_I2C_SDA,
        CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_I2C_SCL,
        400000u,                                        /* 400 kHz */
        &i2c);
    if (err != BLUSYS_OK) {
        printf("i2c_master_open failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("I2C bus opened (SDA=%d, SCL=%d, addr=0x%02X)\n",
           CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_I2C_SDA,
           CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_I2C_SCL,
           CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_I2C_ADDR);

    blusys_gpio_expander_config_t config = {
        .ic          = EXAMPLE_IC,
        .i2c         = i2c,
        .i2c_address = CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_I2C_ADDR,
        .timeout_ms  = EXAMPLE_TIMEOUT_MS,
    };

#endif /* SPI / I2C */

    /* ── Open expander ────────────────────────────────────────────────────── */
    blusys_gpio_expander_t *exp = NULL;
    err = blusys_gpio_expander_open(&config, &exp);
    if (err != BLUSYS_OK) {
        printf("gpio_expander_open failed: %s\n", blusys_err_string(err));
        goto cleanup_bus;
    }
    printf("GPIO expander opened (%d pins)\n", EXAMPLE_PINS);

    /* ── Configure direction: output pins 0–3, input pins 4–(EXAMPLE_PINS-1) */
    for (int pin = 0; pin < EXAMPLE_PINS; pin++) {
        blusys_gpio_expander_dir_t dir = (pin < 4)
                                         ? BLUSYS_GPIO_EXPANDER_OUTPUT
                                         : BLUSYS_GPIO_EXPANDER_INPUT;
        err = blusys_gpio_expander_set_direction(exp, (uint8_t)pin, dir);
        if (err != BLUSYS_OK) {
            printf("set_direction pin %d failed: %s\n", pin, blusys_err_string(err));
            goto cleanup_expander;
        }
    }
    printf("Pins 0–3: OUTPUT | Pins 4–%d: INPUT\n", EXAMPLE_PINS - 1);

    /* ── Main loop: rotating output pattern + port read ──────────────────── */
    for (int i = 0; i < 10; i++) {
        /* Rotate a single bit across the 4 output pins */
        uint16_t out_mask = (uint16_t)(1u << (i % 4));
        err = blusys_gpio_expander_write_port(exp, out_mask);
        if (err != BLUSYS_OK) {
            printf("write_port failed: %s\n", blusys_err_string(err));
            break;
        }

        /* Read all pin states */
        uint16_t port_val = 0;
        err = blusys_gpio_expander_read_port(exp, &port_val);
        if (err != BLUSYS_OK) {
            printf("read_port failed: %s\n", blusys_err_string(err));
            break;
        }

        printf("[%2d] write=0x%04X  read_port=0x%04X  pins:", i, out_mask, port_val);
        for (int pin = 0; pin < EXAMPLE_PINS; pin++) {
            printf(" P%d=%d", pin, (port_val >> pin) & 1u);
        }
        printf("\n");

        vTaskDelay(pdMS_TO_TICKS(500));
    }

cleanup_expander:
    blusys_gpio_expander_close(exp);
    printf("Expander closed\n");

cleanup_bus:
#if defined(CONFIG_BLUSYS_GPIO_EXPANDER_BASIC_IC_MCP23S17)
    blusys_spi_close(spi);
    printf("SPI bus closed\n");
#else
    blusys_i2c_master_close(i2c);
    printf("I2C bus closed\n");
#endif

    printf("Done\n");
}

#else /* !CONFIG_PERIPHERAL_LAB_SCENARIO_GPIO_EXPANDER */
void run_periph_gpio_expander(void) {}
#endif /* CONFIG_PERIPHERAL_LAB_SCENARIO_GPIO_EXPANDER */
