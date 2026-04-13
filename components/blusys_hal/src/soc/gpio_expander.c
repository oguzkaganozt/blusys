#include "blusys/gpio_expander.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

/* ─────────────────────────────────────────── MCP23017/S17 register addresses (IOCON.BANK=0) */

#define MCP23X17_REG_IODIR   0x00u   /* IODIRA; IODIRB is 0x01 (sequential) */
#define MCP23X17_REG_IOCON   0x0Au   /* IOCON (same in BANK=0 and BANK=1 for this address) */
#define MCP23X17_REG_GPIO    0x12u   /* GPIOA; GPIOB is 0x13 (sequential) */
#define MCP23X17_REG_OLAT    0x14u   /* OLATA; OLATB is 0x15 (sequential) */

/* MCP23S17 SPI opcodes */
#define MCP23S17_OPCODE_WRITE(hw_addr)  ((uint8_t)(0x40u | (uint8_t)((hw_addr) << 1u)))
#define MCP23S17_OPCODE_READ(hw_addr)   ((uint8_t)(0x40u | (uint8_t)((hw_addr) << 1u) | 0x01u))

/* ─────────────────────────────────────────────────────────────────── Handle */

struct blusys_gpio_expander {
    blusys_gpio_expander_ic_t  ic;
    blusys_i2c_master_t       *i2c;
    blusys_spi_t              *spi;
    uint16_t                   i2c_address;
    uint8_t                    hw_addr;       /* MCP23S17 A2:A0 */
    int                        timeout_ms;
    uint16_t                   output_latch;  /* cached output register state */
    uint16_t                   dir_mask;      /* cached direction, 1=input 0=output */
    uint8_t                    pin_count;     /* 8 or 16 */
    blusys_lock_t              lock;
};

/* ─────────────────────────────────────── MCP23S17 static SPI helpers */

/* Write a single register via SPI: [opcode_wr, reg, data] */
static blusys_err_t mcp23s17_write(blusys_gpio_expander_t *e, uint8_t reg, uint8_t data)
{
    uint8_t tx[3] = { MCP23S17_OPCODE_WRITE(e->hw_addr), reg, data };
    return blusys_spi_transfer(e->spi, tx, NULL, sizeof(tx));
}

/* Sequential write of two registers: [opcode_wr, reg, d0, d1] (uses SEQOP=0 auto-increment) */
static blusys_err_t mcp23s17_write2(blusys_gpio_expander_t *e, uint8_t reg, uint8_t d0, uint8_t d1)
{
    uint8_t tx[4] = { MCP23S17_OPCODE_WRITE(e->hw_addr), reg, d0, d1 };
    return blusys_spi_transfer(e->spi, tx, NULL, sizeof(tx));
}

/* Read a single register: [opcode_rd, reg, 0x00] → rx[2] */
static blusys_err_t mcp23s17_read(blusys_gpio_expander_t *e, uint8_t reg, uint8_t *out)
{
    uint8_t tx[3] = { MCP23S17_OPCODE_READ(e->hw_addr), reg, 0x00u };
    uint8_t rx[3] = { 0 };
    blusys_err_t err = blusys_spi_transfer(e->spi, tx, rx, sizeof(tx));
    if (err == BLUSYS_OK) {
        *out = rx[2];
    }
    return err;
}

/* Sequential read of two registers: [opcode_rd, reg, 0x00, 0x00] → rx[2], rx[3] */
static blusys_err_t mcp23s17_read2(blusys_gpio_expander_t *e, uint8_t reg, uint8_t *d0, uint8_t *d1)
{
    uint8_t tx[4] = { MCP23S17_OPCODE_READ(e->hw_addr), reg, 0x00u, 0x00u };
    uint8_t rx[4] = { 0 };
    blusys_err_t err = blusys_spi_transfer(e->spi, tx, rx, sizeof(tx));
    if (err == BLUSYS_OK) {
        *d0 = rx[2];
        *d1 = rx[3];
    }
    return err;
}

/* ──────────────────────────────────────────── PCF8574 helpers */

/* Write the port byte. For PCF8574/A the entire port is a single byte. */
static blusys_err_t pcf857x_write_port(blusys_gpio_expander_t *e)
{
    uint8_t byte = (uint8_t)(e->output_latch & 0xFFu);
    return blusys_i2c_master_write(e->i2c, e->i2c_address, &byte, 1, e->timeout_ms);
}

/* Read the port byte. */
static blusys_err_t pcf857x_read_port(blusys_gpio_expander_t *e, uint8_t *out)
{
    return blusys_i2c_master_read(e->i2c, e->i2c_address, out, 1, e->timeout_ms);
}

/* ──────────────────────────────────────────── MCP23017 helpers */

/* Write both IODIR registers in one sequential transaction. */
static blusys_err_t mcp23017_write_iodir(blusys_gpio_expander_t *e)
{
    uint8_t buf[3] = {
        MCP23X17_REG_IODIR,
        (uint8_t)(e->dir_mask & 0xFFu),
        (uint8_t)(e->dir_mask >> 8u),
    };
    return blusys_i2c_master_write(e->i2c, e->i2c_address, buf, sizeof(buf), e->timeout_ms);
}

/* Write both OLAT registers in one sequential transaction. */
static blusys_err_t mcp23017_write_olat(blusys_gpio_expander_t *e)
{
    uint8_t buf[3] = {
        MCP23X17_REG_OLAT,
        (uint8_t)(e->output_latch & 0xFFu),
        (uint8_t)(e->output_latch >> 8u),
    };
    return blusys_i2c_master_write(e->i2c, e->i2c_address, buf, sizeof(buf), e->timeout_ms);
}

/* Read both GPIO registers (live pin state). */
static blusys_err_t mcp23017_read_gpio(blusys_gpio_expander_t *e, uint16_t *out)
{
    uint8_t reg = MCP23X17_REG_GPIO;
    uint8_t gpio[2] = { 0 };
    blusys_err_t err = blusys_i2c_master_write_read(e->i2c, e->i2c_address,
                                                     &reg, 1,
                                                     gpio, 2,
                                                     e->timeout_ms);
    if (err == BLUSYS_OK) {
        *out = (uint16_t)gpio[0] | ((uint16_t)gpio[1] << 8u);
    }
    return err;
}

/* ─────────────────────────────────────────────────────────── open / close */

blusys_err_t blusys_gpio_expander_open(const blusys_gpio_expander_config_t *config,
                                        blusys_gpio_expander_t **out_handle)
{
    if (config == NULL || out_handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    /* Validate IC type and transport pairing */
    switch (config->ic) {
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574:
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574A:
        case BLUSYS_GPIO_EXPANDER_IC_MCP23017:
            if (config->i2c == NULL || config->spi != NULL) {
                return BLUSYS_ERR_INVALID_ARG;
            }
            break;
        case BLUSYS_GPIO_EXPANDER_IC_MCP23S17:
            if (config->spi == NULL || config->i2c != NULL) {
                return BLUSYS_ERR_INVALID_ARG;
            }
            if (config->hw_addr > 7u) {
                return BLUSYS_ERR_INVALID_ARG;
            }
            break;
        default:
            return BLUSYS_ERR_INVALID_ARG;
    }

    /* Validate I2C address ranges */
    if (config->ic == BLUSYS_GPIO_EXPANDER_IC_PCF8574) {
        if (config->i2c_address < 0x20u || config->i2c_address > 0x27u) {
            return BLUSYS_ERR_INVALID_ARG;
        }
    } else if (config->ic == BLUSYS_GPIO_EXPANDER_IC_PCF8574A) {
        if (config->i2c_address < 0x38u || config->i2c_address > 0x3Fu) {
            return BLUSYS_ERR_INVALID_ARG;
        }
    } else if (config->ic == BLUSYS_GPIO_EXPANDER_IC_MCP23017) {
        if (config->i2c_address < 0x20u || config->i2c_address > 0x27u) {
            return BLUSYS_ERR_INVALID_ARG;
        }
    }

    blusys_gpio_expander_t *exp = calloc(1, sizeof(*exp));
    if (exp == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    exp->ic          = config->ic;
    exp->i2c         = config->i2c;
    exp->spi         = config->spi;
    exp->i2c_address = config->i2c_address;
    exp->hw_addr     = config->hw_addr;
    exp->timeout_ms  = config->timeout_ms;
    exp->pin_count   = (config->ic == BLUSYS_GPIO_EXPANDER_IC_PCF8574 ||
                        config->ic == BLUSYS_GPIO_EXPANDER_IC_PCF8574A) ? 8u : 16u;

    blusys_err_t err = blusys_lock_init(&exp->lock);
    if (err != BLUSYS_OK) {
        free(exp);
        return err;
    }

    /* IC-specific hardware initialisation */
    switch (config->ic) {
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574:
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574A: {
            /* All pins start as inputs (weak pull-up): write 0xFF */
            exp->output_latch = 0x00FFu;
            exp->dir_mask     = 0x00FFu;
            err = pcf857x_write_port(exp);
            break;
        }
        case BLUSYS_GPIO_EXPANDER_IC_MCP23017: {
            /* Force IOCON.BANK=0 (default but write explicitly for robustness) */
            uint8_t iocon_buf[2] = { MCP23X17_REG_IOCON, 0x00u };
            err = blusys_i2c_master_write(exp->i2c, exp->i2c_address,
                                          iocon_buf, sizeof(iocon_buf), exp->timeout_ms);
            if (err != BLUSYS_OK) { break; }

            /* Read back IODIRA+B */
            uint8_t reg_iodir = MCP23X17_REG_IODIR;
            uint8_t iodir[2]  = { 0 };
            err = blusys_i2c_master_write_read(exp->i2c, exp->i2c_address,
                                               &reg_iodir, 1,
                                               iodir, 2, exp->timeout_ms);
            if (err != BLUSYS_OK) { break; }
            exp->dir_mask = (uint16_t)iodir[0] | ((uint16_t)iodir[1] << 8u);

            /* Read back OLATA+B */
            uint8_t reg_olat = MCP23X17_REG_OLAT;
            uint8_t olat[2]  = { 0 };
            err = blusys_i2c_master_write_read(exp->i2c, exp->i2c_address,
                                               &reg_olat, 1,
                                               olat, 2, exp->timeout_ms);
            if (err != BLUSYS_OK) { break; }
            exp->output_latch = (uint16_t)olat[0] | ((uint16_t)olat[1] << 8u);
            break;
        }
        case BLUSYS_GPIO_EXPANDER_IC_MCP23S17: {
            /* Force IOCON.BANK=0 */
            err = mcp23s17_write(exp, MCP23X17_REG_IOCON, 0x00u);
            if (err != BLUSYS_OK) { break; }

            /* Read back IODIR A+B */
            uint8_t d0, d1;
            err = mcp23s17_read2(exp, MCP23X17_REG_IODIR, &d0, &d1);
            if (err != BLUSYS_OK) { break; }
            exp->dir_mask = (uint16_t)d0 | ((uint16_t)d1 << 8u);

            /* Read back OLAT A+B */
            err = mcp23s17_read2(exp, MCP23X17_REG_OLAT, &d0, &d1);
            if (err != BLUSYS_OK) { break; }
            exp->output_latch = (uint16_t)d0 | ((uint16_t)d1 << 8u);
            break;
        }
        default:
            err = BLUSYS_ERR_INVALID_ARG;
            break;
    }

    if (err != BLUSYS_OK) {
        blusys_lock_deinit(&exp->lock);
        free(exp);
        return err;
    }

    *out_handle = exp;
    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_expander_close(blusys_gpio_expander_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t lock_err = blusys_lock_take(&handle->lock, BLUSYS_TIMEOUT_FOREVER);
    if (lock_err != BLUSYS_OK) {
        return lock_err;
    }

    /* Best-effort hardware reset; ignore errors */
    switch (handle->ic) {
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574:
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574A: {
            uint8_t byte = 0xFFu;
            blusys_i2c_master_write(handle->i2c, handle->i2c_address, &byte, 1, handle->timeout_ms);
            break;
        }
        case BLUSYS_GPIO_EXPANDER_IC_MCP23017: {
            /* All inputs, latches cleared */
            uint8_t iodir_buf[3] = { MCP23X17_REG_IODIR, 0xFFu, 0xFFu };
            blusys_i2c_master_write(handle->i2c, handle->i2c_address,
                                    iodir_buf, sizeof(iodir_buf), handle->timeout_ms);
            uint8_t olat_buf[3] = { MCP23X17_REG_OLAT, 0x00u, 0x00u };
            blusys_i2c_master_write(handle->i2c, handle->i2c_address,
                                    olat_buf, sizeof(olat_buf), handle->timeout_ms);
            break;
        }
        case BLUSYS_GPIO_EXPANDER_IC_MCP23S17: {
            mcp23s17_write2(handle, MCP23X17_REG_IODIR, 0xFFu, 0xFFu);
            mcp23s17_write2(handle, MCP23X17_REG_OLAT,  0x00u, 0x00u);
            break;
        }
        default:
            break;
    }

    blusys_lock_give(&handle->lock);
    blusys_lock_deinit(&handle->lock);
    free(handle);
    return BLUSYS_OK;
}

/* ─────────────────────────────────────────── set_direction */

blusys_err_t blusys_gpio_expander_set_direction(blusys_gpio_expander_t *handle,
                                                 uint8_t pin,
                                                 blusys_gpio_expander_dir_t dir)
{
    if (handle == NULL || pin >= handle->pin_count) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (dir != BLUSYS_GPIO_EXPANDER_OUTPUT && dir != BLUSYS_GPIO_EXPANDER_INPUT) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, handle->timeout_ms);
    if (err != BLUSYS_OK) { return err; }

    uint16_t bit = (uint16_t)(1u << pin);
    if (dir == BLUSYS_GPIO_EXPANDER_INPUT) {
        handle->dir_mask |= bit;
    } else {
        handle->dir_mask &= (uint16_t)~bit;
    }

    switch (handle->ic) {
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574:
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574A:
            /* INPUT: latch bit must stay high for weak pull-up */
            if (dir == BLUSYS_GPIO_EXPANDER_INPUT) {
                handle->output_latch |= bit;
            }
            err = pcf857x_write_port(handle);
            break;
        case BLUSYS_GPIO_EXPANDER_IC_MCP23017:
            err = mcp23017_write_iodir(handle);
            break;
        case BLUSYS_GPIO_EXPANDER_IC_MCP23S17:
            err = mcp23s17_write2(handle, MCP23X17_REG_IODIR,
                                  (uint8_t)(handle->dir_mask & 0xFFu),
                                  (uint8_t)(handle->dir_mask >> 8u));
            break;
        default:
            err = BLUSYS_ERR_INVALID_ARG;
            break;
    }

    blusys_lock_give(&handle->lock);
    return err;
}

/* ─────────────────────────────────────────── write_pin / write_port */

blusys_err_t blusys_gpio_expander_write_pin(blusys_gpio_expander_t *handle,
                                             uint8_t pin,
                                             bool level)
{
    if (handle == NULL || pin >= handle->pin_count) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, handle->timeout_ms);
    if (err != BLUSYS_OK) { return err; }

    uint16_t bit = (uint16_t)(1u << pin);
    if (level) {
        handle->output_latch |= bit;
    } else {
        handle->output_latch &= (uint16_t)~bit;
    }

    switch (handle->ic) {
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574:
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574A:
            /* Enforce invariant: input-pin bits must stay high */
            handle->output_latch |= (handle->dir_mask & 0x00FFu);
            err = pcf857x_write_port(handle);
            break;
        case BLUSYS_GPIO_EXPANDER_IC_MCP23017:
            err = mcp23017_write_olat(handle);
            break;
        case BLUSYS_GPIO_EXPANDER_IC_MCP23S17:
            err = mcp23s17_write2(handle, MCP23X17_REG_OLAT,
                                  (uint8_t)(handle->output_latch & 0xFFu),
                                  (uint8_t)(handle->output_latch >> 8u));
            break;
        default:
            err = BLUSYS_ERR_INVALID_ARG;
            break;
    }

    blusys_lock_give(&handle->lock);
    return err;
}

blusys_err_t blusys_gpio_expander_write_port(blusys_gpio_expander_t *handle,
                                              uint16_t values)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, handle->timeout_ms);
    if (err != BLUSYS_OK) { return err; }

    switch (handle->ic) {
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574:
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574A: {
            uint16_t new_latch = (values & 0x00FFu);
            /* Enforce invariant: input-pin bits must stay high */
            new_latch |= (handle->dir_mask & 0x00FFu);
            handle->output_latch = new_latch;
            err = pcf857x_write_port(handle);
            break;
        }
        case BLUSYS_GPIO_EXPANDER_IC_MCP23017:
            handle->output_latch = values;
            err = mcp23017_write_olat(handle);
            break;
        case BLUSYS_GPIO_EXPANDER_IC_MCP23S17:
            handle->output_latch = values;
            err = mcp23s17_write2(handle, MCP23X17_REG_OLAT,
                                  (uint8_t)(values & 0xFFu),
                                  (uint8_t)(values >> 8u));
            break;
        default:
            err = BLUSYS_ERR_INVALID_ARG;
            break;
    }

    blusys_lock_give(&handle->lock);
    return err;
}

/* ─────────────────────────────────────────── read_pin / read_port */

blusys_err_t blusys_gpio_expander_read_pin(blusys_gpio_expander_t *handle,
                                            uint8_t pin,
                                            bool *out_level)
{
    if (handle == NULL || pin >= handle->pin_count || out_level == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, handle->timeout_ms);
    if (err != BLUSYS_OK) { return err; }

    uint16_t port = 0;
    switch (handle->ic) {
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574:
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574A: {
            uint8_t byte = 0;
            err = pcf857x_read_port(handle, &byte);
            port = (uint16_t)byte;
            break;
        }
        case BLUSYS_GPIO_EXPANDER_IC_MCP23017:
            err = mcp23017_read_gpio(handle, &port);
            break;
        case BLUSYS_GPIO_EXPANDER_IC_MCP23S17: {
            uint8_t d0, d1;
            err = mcp23s17_read2(handle, MCP23X17_REG_GPIO, &d0, &d1);
            port = (uint16_t)d0 | ((uint16_t)d1 << 8u);
            break;
        }
        default:
            err = BLUSYS_ERR_INVALID_ARG;
            break;
    }

    if (err == BLUSYS_OK) {
        *out_level = (bool)((port >> pin) & 1u);
    }

    blusys_lock_give(&handle->lock);
    return err;
}

blusys_err_t blusys_gpio_expander_read_port(blusys_gpio_expander_t *handle,
                                             uint16_t *out_values)
{
    if (handle == NULL || out_values == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, handle->timeout_ms);
    if (err != BLUSYS_OK) { return err; }

    switch (handle->ic) {
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574:
        case BLUSYS_GPIO_EXPANDER_IC_PCF8574A: {
            uint8_t byte = 0;
            err = pcf857x_read_port(handle, &byte);
            if (err == BLUSYS_OK) {
                *out_values = (uint16_t)byte;
            }
            break;
        }
        case BLUSYS_GPIO_EXPANDER_IC_MCP23017:
            err = mcp23017_read_gpio(handle, out_values);
            break;
        case BLUSYS_GPIO_EXPANDER_IC_MCP23S17: {
            uint8_t d0, d1;
            err = mcp23s17_read2(handle, MCP23X17_REG_GPIO, &d0, &d1);
            if (err == BLUSYS_OK) {
                *out_values = (uint16_t)d0 | ((uint16_t)d1 << 8u);
            }
            break;
        }
        default:
            err = BLUSYS_ERR_INVALID_ARG;
            break;
    }

    blusys_lock_give(&handle->lock);
    return err;
}
