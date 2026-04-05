#ifndef BLUSYS_RMT_H
#define BLUSYS_RMT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Shared pulse type */

typedef struct {
    bool level;
    uint32_t duration_us;
} blusys_rmt_pulse_t;

/* TX */

typedef struct blusys_rmt blusys_rmt_t;

blusys_err_t blusys_rmt_open(int pin, bool idle_level, blusys_rmt_t **out_rmt);
blusys_err_t blusys_rmt_close(blusys_rmt_t *rmt);
blusys_err_t blusys_rmt_write(blusys_rmt_t *rmt,
                              const blusys_rmt_pulse_t *pulses,
                              size_t pulse_count,
                              int timeout_ms);

/* RX */

typedef struct blusys_rmt_rx blusys_rmt_rx_t;

typedef struct {
    uint32_t signal_range_min_ns;   /* glitch filter: pulses shorter than this are ignored */
    uint32_t signal_range_max_ns;   /* idle threshold: silence longer than this ends the frame */
} blusys_rmt_rx_config_t;

blusys_err_t blusys_rmt_rx_open(int pin,
                                 const blusys_rmt_rx_config_t *config,
                                 blusys_rmt_rx_t **out_rmt_rx);
blusys_err_t blusys_rmt_rx_close(blusys_rmt_rx_t *rmt_rx);
blusys_err_t blusys_rmt_rx_read(blusys_rmt_rx_t *rmt_rx,
                                 blusys_rmt_pulse_t *pulses,
                                 size_t max_pulses,
                                 size_t *out_count,
                                 int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
