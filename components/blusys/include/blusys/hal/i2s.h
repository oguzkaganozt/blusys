#ifndef BLUSYS_I2S_H
#define BLUSYS_I2S_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TX */

typedef struct blusys_i2s_tx blusys_i2s_tx_t;

typedef struct {
    int port;
    int bclk_pin;
    int ws_pin;
    int dout_pin;
    int mclk_pin;
    uint32_t sample_rate_hz;
} blusys_i2s_tx_config_t;

blusys_err_t blusys_i2s_tx_open(const blusys_i2s_tx_config_t *config,
                                blusys_i2s_tx_t **out_i2s);
blusys_err_t blusys_i2s_tx_close(blusys_i2s_tx_t *i2s);
blusys_err_t blusys_i2s_tx_start(blusys_i2s_tx_t *i2s);
blusys_err_t blusys_i2s_tx_stop(blusys_i2s_tx_t *i2s);
blusys_err_t blusys_i2s_tx_write(blusys_i2s_tx_t *i2s,
                                 const void *data,
                                 size_t size,
                                 int timeout_ms);

/* RX */

typedef struct blusys_i2s_rx blusys_i2s_rx_t;

typedef struct {
    int port;
    int bclk_pin;
    int ws_pin;
    int din_pin;
    int mclk_pin;
    uint32_t sample_rate_hz;
} blusys_i2s_rx_config_t;

blusys_err_t blusys_i2s_rx_open(const blusys_i2s_rx_config_t *config,
                                blusys_i2s_rx_t **out_i2s);
blusys_err_t blusys_i2s_rx_close(blusys_i2s_rx_t *i2s);
blusys_err_t blusys_i2s_rx_start(blusys_i2s_rx_t *i2s);
blusys_err_t blusys_i2s_rx_stop(blusys_i2s_rx_t *i2s);
blusys_err_t blusys_i2s_rx_read(blusys_i2s_rx_t *i2s,
                                void *buf,
                                size_t size,
                                size_t *bytes_read,
                                int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
