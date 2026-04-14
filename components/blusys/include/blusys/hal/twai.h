#ifndef BLUSYS_TWAI_H
#define BLUSYS_TWAI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_twai blusys_twai_t;

typedef struct {
    uint32_t id;
    bool remote_frame;
    uint8_t dlc;
    uint8_t data[8];
    size_t data_len;
} blusys_twai_frame_t;

typedef bool (*blusys_twai_rx_callback_t)(blusys_twai_t *twai,
                                          const blusys_twai_frame_t *frame,
                                          void *user_ctx);

blusys_err_t blusys_twai_open(int tx_pin,
                              int rx_pin,
                              uint32_t bitrate,
                              blusys_twai_t **out_twai);
blusys_err_t blusys_twai_close(blusys_twai_t *twai);
blusys_err_t blusys_twai_start(blusys_twai_t *twai);
blusys_err_t blusys_twai_stop(blusys_twai_t *twai);
blusys_err_t blusys_twai_write(blusys_twai_t *twai,
                               const blusys_twai_frame_t *frame,
                               int timeout_ms);
blusys_err_t blusys_twai_set_rx_callback(blusys_twai_t *twai,
                                         blusys_twai_rx_callback_t callback,
                                         void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif
