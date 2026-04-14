#ifndef BLUSYS_GPIO_EXPANDER_H
#define BLUSYS_GPIO_EXPANDER_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"
#include "blusys/hal/i2c.h"
#include "blusys/hal/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque GPIO expander handle.
 */
typedef struct blusys_gpio_expander blusys_gpio_expander_t;

/**
 * @brief Supported GPIO expander IC types.
 */
typedef enum {
    BLUSYS_GPIO_EXPANDER_IC_PCF8574,    /**< PCF8574:  8 pins, I2C, address 0x20–0x27 */
    BLUSYS_GPIO_EXPANDER_IC_PCF8574A,   /**< PCF8574A: 8 pins, I2C, address 0x38–0x3F */
    BLUSYS_GPIO_EXPANDER_IC_MCP23017,   /**< MCP23017: 16 pins, I2C, address 0x20–0x27 */
    BLUSYS_GPIO_EXPANDER_IC_MCP23S17,   /**< MCP23S17: 16 pins, SPI, hw_addr 0–7 */
} blusys_gpio_expander_ic_t;

/**
 * @brief Pin direction.
 */
typedef enum {
    BLUSYS_GPIO_EXPANDER_OUTPUT = 0,    /**< Pin driven as output */
    BLUSYS_GPIO_EXPANDER_INPUT  = 1,    /**< Pin read as input */
} blusys_gpio_expander_dir_t;

/**
 * @brief Configuration for blusys_gpio_expander_open().
 *
 * For I2C ICs (PCF8574, PCF8574A, MCP23017): set @p i2c and @p i2c_address; leave @p spi NULL.
 * For MCP23S17: set @p spi and @p hw_addr; leave @p i2c NULL.
 *
 * The caller owns the bus handles and must keep them open for the lifetime of the expander.
 * @p timeout_ms applies to every I2C transaction; ignored for SPI (deterministic).
 */
typedef struct {
    blusys_gpio_expander_ic_t  ic;           /**< IC type */
    blusys_i2c_master_t       *i2c;          /**< I2C bus handle (PCF8574/A, MCP23017) */
    uint16_t                   i2c_address;  /**< 7-bit I2C address */
    blusys_spi_t              *spi;          /**< SPI bus handle (MCP23S17) */
    uint8_t                    hw_addr;      /**< MCP23S17 hardware address, A2:A0 (0–7) */
    int                        timeout_ms;   /**< Per-transaction timeout (ms); use BLUSYS_TIMEOUT_FOREVER to block */
} blusys_gpio_expander_config_t;

/**
 * @brief Open a GPIO expander.
 *
 * Allocates a handle, validates config, and performs IC-specific initialisation:
 * - PCF8574/A: writes 0xFF to all pins (enables weak pull-up, all inputs).
 * - MCP23017/S17: forces IOCON.BANK=0 then reads back IODIR and OLAT into the cache.
 *
 * The caller retains ownership of @p config->i2c or @p config->spi and must not close
 * the bus until after calling blusys_gpio_expander_close().
 *
 * @param[in]  config     Expander configuration.
 * @param[out] out_handle Written with the new handle on success.
 * @return BLUSYS_OK on success.
 * @return BLUSYS_ERR_INVALID_ARG if config or out_handle is NULL, IC type is unknown,
 *         transport handle is wrong for the selected IC, or address is out of range.
 * @return BLUSYS_ERR_NO_MEM if heap allocation fails.
 * @return BLUSYS_ERR_IO on I2C/SPI communication failure.
 * @return BLUSYS_ERR_TIMEOUT on I2C transaction timeout.
 */
blusys_err_t blusys_gpio_expander_open(const blusys_gpio_expander_config_t *config,
                                        blusys_gpio_expander_t **out_handle);

/**
 * @brief Close a GPIO expander and free all resources.
 *
 * Performs a best-effort hardware reset (PCF854x: all pins high; MCP2301x: IODIR=0xFF, OLAT=0x00)
 * then frees the handle. Does NOT close the underlying I2C or SPI bus — that is the caller's
 * responsibility.
 *
 * @param[in] handle Handle returned by blusys_gpio_expander_open().
 * @return BLUSYS_OK on success.
 * @return BLUSYS_ERR_INVALID_ARG if handle is NULL.
 */
blusys_err_t blusys_gpio_expander_close(blusys_gpio_expander_t *handle);

/**
 * @brief Set the direction of a single pin.
 *
 * For PCF8574/A, direction INPUT sets the output latch bit to 1 (enabling the weak
 * pull-up) so external circuitry can drive the pin low for reading. There is no dedicated
 * direction register on these ICs.
 *
 * Pin numbering: 0–7 for 8-pin ICs. For 16-pin ICs: 0–7 = port A, 8–15 = port B.
 *
 * @param[in] handle Handle returned by blusys_gpio_expander_open().
 * @param[in] pin    Pin index (0–7 for 8-pin ICs, 0–15 for 16-pin ICs).
 * @param[in] dir    Pin direction.
 * @return BLUSYS_OK on success.
 * @return BLUSYS_ERR_INVALID_ARG if handle is NULL, pin is out of range, or dir is invalid.
 * @return BLUSYS_ERR_IO / BLUSYS_ERR_TIMEOUT on bus error.
 */
blusys_err_t blusys_gpio_expander_set_direction(blusys_gpio_expander_t *handle,
                                                 uint8_t pin,
                                                 blusys_gpio_expander_dir_t dir);

/**
 * @brief Write the level of a single output pin.
 *
 * Updates the internal output latch and writes to hardware. For PCF8574/A the input-pin
 * bits are kept high automatically to preserve the weak pull-up invariant.
 *
 * @param[in] handle Handle returned by blusys_gpio_expander_open().
 * @param[in] pin    Pin index.
 * @param[in] level  true = high, false = low.
 * @return BLUSYS_OK on success.
 * @return BLUSYS_ERR_INVALID_ARG if handle is NULL or pin is out of range.
 * @return BLUSYS_ERR_IO / BLUSYS_ERR_TIMEOUT on bus error.
 */
blusys_err_t blusys_gpio_expander_write_pin(blusys_gpio_expander_t *handle,
                                             uint8_t pin,
                                             bool level);

/**
 * @brief Read the current logic level of a single pin.
 *
 * Always reads live hardware state, not the cached output latch.
 *
 * @param[in]  handle    Handle returned by blusys_gpio_expander_open().
 * @param[in]  pin       Pin index.
 * @param[out] out_level Written with the pin level on success.
 * @return BLUSYS_OK on success.
 * @return BLUSYS_ERR_INVALID_ARG if any pointer is NULL or pin is out of range.
 * @return BLUSYS_ERR_IO / BLUSYS_ERR_TIMEOUT on bus error.
 */
blusys_err_t blusys_gpio_expander_read_pin(blusys_gpio_expander_t *handle,
                                            uint8_t pin,
                                            bool *out_level);

/**
 * @brief Write all output pins in a single bus transaction.
 *
 * @p values is a 16-bit bitmask (bit N = pin N). The upper 8 bits are ignored for 8-pin ICs.
 * For PCF8574/A the bits corresponding to input-configured pins are forced to 1 to maintain
 * the weak pull-up invariant.
 *
 * @param[in] handle Handle returned by blusys_gpio_expander_open().
 * @param[in] values Pin output bitmask.
 * @return BLUSYS_OK on success.
 * @return BLUSYS_ERR_INVALID_ARG if handle is NULL.
 * @return BLUSYS_ERR_IO / BLUSYS_ERR_TIMEOUT on bus error.
 */
blusys_err_t blusys_gpio_expander_write_port(blusys_gpio_expander_t *handle,
                                              uint16_t values);

/**
 * @brief Read all pin states in a single bus transaction.
 *
 * @p out_values receives a 16-bit bitmask (bit N = pin N). The upper 8 bits are always 0
 * for 8-pin ICs. Always reflects live hardware state.
 *
 * @param[in]  handle     Handle returned by blusys_gpio_expander_open().
 * @param[out] out_values Written with the port bitmask on success.
 * @return BLUSYS_OK on success.
 * @return BLUSYS_ERR_INVALID_ARG if either pointer is NULL.
 * @return BLUSYS_ERR_IO / BLUSYS_ERR_TIMEOUT on bus error.
 */
blusys_err_t blusys_gpio_expander_read_port(blusys_gpio_expander_t *handle,
                                             uint16_t *out_values);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_GPIO_EXPANDER_H */
