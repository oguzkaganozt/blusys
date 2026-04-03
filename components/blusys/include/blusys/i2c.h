#ifndef BLUSYS_I2C_H
#define BLUSYS_I2C_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_i2c_master blusys_i2c_master_t;

blusys_err_t blusys_i2c_master_open(int port,
                                    int sda_pin,
                                    int scl_pin,
                                    uint32_t freq_hz,
                                    blusys_i2c_master_t **out_i2c);
blusys_err_t blusys_i2c_master_close(blusys_i2c_master_t *i2c);
blusys_err_t blusys_i2c_master_probe(blusys_i2c_master_t *i2c, uint16_t device_address, int timeout_ms);
blusys_err_t blusys_i2c_master_write(blusys_i2c_master_t *i2c,
                                     uint16_t device_address,
                                     const void *data,
                                     size_t size,
                                     int timeout_ms);
blusys_err_t blusys_i2c_master_read(blusys_i2c_master_t *i2c,
                                    uint16_t device_address,
                                    void *data,
                                    size_t size,
                                    int timeout_ms);
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
