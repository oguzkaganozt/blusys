/**
 * @file i2c.h
 * @brief Blocking master-mode I2C bus: probe, write, read, write-then-read.
 *
 * 7-bit addressing only. Per-transfer device selection through the
 * `device_address` argument; no per-device handle is needed. See
 * docs/hal/i2c.md for common pitfalls (external pull-ups, pin selection).
 */

#ifndef BLUSYS_I2C_H
#define BLUSYS_I2C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for a configured I2C master bus. */
typedef struct blusys_i2c_master blusys_i2c_master_t;

/**
 * @brief Scan callback: fires once per responding device during ::blusys_i2c_master_scan.
 * @param device_address 7-bit address that responded.
 * @param user_ctx        User context pointer provided at scan time.
 * @return `false` to continue scanning, `true` to stop early.
 */
typedef bool (*blusys_i2c_scan_callback_t)(uint16_t device_address, void *user_ctx);

/**
 * @brief Open an I2C master bus. Internal pull-ups are enabled.
 *
 * External pull-ups are still recommended on a bare bus.
 *
 * @param port      I2C port number (0 or 1).
 * @param sda_pin   GPIO for SDA.
 * @param scl_pin   GPIO for SCL.
 * @param freq_hz   Bus clock frequency (e.g. 100000 or 400000).
 * @param out_i2c   Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.
 */
blusys_err_t blusys_i2c_master_open(int port,
                                    int sda_pin,
                                    int scl_pin,
                                    uint32_t freq_hz,
                                    blusys_i2c_master_t **out_i2c);

/**
 * @brief Release the I2C bus and free the handle.
 * @param i2c Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p i2c is NULL.
 */
blusys_err_t blusys_i2c_master_close(blusys_i2c_master_t *i2c);

/**
 * @brief Check whether a device responds at the given 7-bit address.
 *
 * @param i2c             Handle.
 * @param device_address  7-bit device address.
 * @param timeout_ms      Milliseconds to wait.
 * @return `BLUSYS_OK` if the device ACKs,
 *         `BLUSYS_ERR_IO` if no ACK (device absent),
 *         `BLUSYS_ERR_TIMEOUT` on bus timeout.
 */
blusys_err_t blusys_i2c_master_probe(blusys_i2c_master_t *i2c, uint16_t device_address, int timeout_ms);

/**
 * @brief Probe each address in [@p start_address, @p end_address] and invoke @p callback for responders.
 *
 * @param i2c            Handle.
 * @param start_address  First 7-bit address to probe.
 * @param end_address    Last 7-bit address to probe (inclusive).
 * @param timeout_ms     Per-address probe timeout.
 * @param callback       Called for each responding device; may be NULL.
 * @param user_ctx       Opaque pointer passed back to the callback unchanged.
 * @param out_count      Total number of responding devices found; may be NULL.
 * @return `BLUSYS_OK` on completion.
 */
blusys_err_t blusys_i2c_master_scan(blusys_i2c_master_t *i2c,
                                    uint16_t start_address,
                                    uint16_t end_address,
                                    int timeout_ms,
                                    blusys_i2c_scan_callback_t callback,
                                    void *user_ctx,
                                    size_t *out_count);

/**
 * @brief Send bytes to a device.
 * @param i2c             Handle.
 * @param device_address  7-bit device address.
 * @param data            Bytes to send.
 * @param size            Number of bytes.
 * @param timeout_ms      Milliseconds to wait.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` on bus timeout, `BLUSYS_ERR_IO` on NACK.
 */
blusys_err_t blusys_i2c_master_write(blusys_i2c_master_t *i2c,
                                     uint16_t device_address,
                                     const void *data,
                                     size_t size,
                                     int timeout_ms);

/**
 * @brief Read bytes from a device.
 * @param i2c             Handle.
 * @param device_address  7-bit device address.
 * @param data            Destination buffer.
 * @param size            Number of bytes to read.
 * @param timeout_ms      Milliseconds to wait.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` on bus timeout, `BLUSYS_ERR_IO` on NACK.
 */
blusys_err_t blusys_i2c_master_read(blusys_i2c_master_t *i2c,
                                    uint16_t device_address,
                                    void *data,
                                    size_t size,
                                    int timeout_ms);

/**
 * @brief Write then read in one atomic transaction (repeated start between phases).
 *
 * Typical use: write a register address, read the register value.
 *
 * @param i2c             Handle.
 * @param device_address  7-bit device address.
 * @param tx_data         Bytes to send first.
 * @param tx_size         Number of TX bytes.
 * @param rx_data         Destination for the read phase.
 * @param rx_size         Number of RX bytes.
 * @param timeout_ms      Milliseconds to wait for the whole transaction.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`, `BLUSYS_ERR_IO` on NACK.
 */
blusys_err_t blusys_i2c_master_write_read(blusys_i2c_master_t *i2c,
                                          uint16_t device_address,
                                          const void *tx_data,
                                          size_t tx_size,
                                          void *rx_data,
                                          size_t rx_size,
                                          int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
