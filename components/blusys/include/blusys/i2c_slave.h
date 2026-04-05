#ifndef BLUSYS_I2C_SLAVE_H
#define BLUSYS_I2C_SLAVE_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_i2c_slave blusys_i2c_slave_t;

blusys_err_t blusys_i2c_slave_open(int sda_pin,
                                    int scl_pin,
                                    uint16_t addr,
                                    uint32_t tx_buf_size,
                                    blusys_i2c_slave_t **out_slave);
blusys_err_t blusys_i2c_slave_close(blusys_i2c_slave_t *slave);
blusys_err_t blusys_i2c_slave_receive(blusys_i2c_slave_t *slave,
                                       uint8_t *buf,
                                       size_t size,
                                       size_t *bytes_received,
                                       int timeout_ms);
blusys_err_t blusys_i2c_slave_transmit(blusys_i2c_slave_t *slave,
                                        const uint8_t *buf,
                                        size_t size,
                                        int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
