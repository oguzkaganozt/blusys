/**
 * @file i2c_slave.h
 * @brief Slave-mode I2C: receive bytes from a master and transmit on request.
 *
 * 7-bit addressing only. Port selection is automatic. See
 * docs/hal/i2c.md (Slave Mode section) for ISR notes and limitations.
 */

#ifndef BLUSYS_I2C_SLAVE_H
#define BLUSYS_I2C_SLAVE_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for an I2C slave device. */
typedef struct blusys_i2c_slave blusys_i2c_slave_t;

/**
 * @brief Open an I2C slave device. Port selection is automatic.
 *
 * @param sda_pin      GPIO for SDA.
 * @param scl_pin      GPIO for SCL.
 * @param addr         7-bit slave address.
 * @param tx_buf_size  Size of the internal TX buffer in bytes; must be > 0.
 * @param out_slave    Output handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for invalid pins, zero buffer size,
 *         or NULL pointer.
 */
blusys_err_t blusys_i2c_slave_open(int sda_pin,
                                    int scl_pin,
                                    uint16_t addr,
                                    uint32_t tx_buf_size,
                                    blusys_i2c_slave_t **out_slave);

/**
 * @brief Release the I2C slave and free its handle.
 * @param slave Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p slave is NULL.
 */
blusys_err_t blusys_i2c_slave_close(blusys_i2c_slave_t *slave);

/**
 * @brief Block until @p size bytes are received from the master or timeout.
 *
 * @param slave           Handle.
 * @param buf             Receive buffer.
 * @param size            Number of bytes to wait for.
 * @param bytes_received  Actual bytes received.
 * @param timeout_ms      Milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` if data doesn't arrive in time.
 * @warning Task-safe; must not be called from an ISR context.
 */
blusys_err_t blusys_i2c_slave_receive(blusys_i2c_slave_t *slave,
                                       uint8_t *buf,
                                       size_t size,
                                       size_t *bytes_received,
                                       int timeout_ms);

/**
 * @brief Queue bytes into the TX buffer for the master to read.
 *
 * Blocks until the bytes are queued or the timeout expires.
 *
 * @param slave       Handle.
 * @param buf         Bytes to queue.
 * @param size        Number of bytes.
 * @param timeout_ms  Milliseconds to wait.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` on queue failure.
 */
blusys_err_t blusys_i2c_slave_transmit(blusys_i2c_slave_t *slave,
                                        const uint8_t *buf,
                                        size_t size,
                                        int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
