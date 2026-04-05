#ifndef BLUSYS_RMT_H
#define BLUSYS_RMT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_rmt blusys_rmt_t;

typedef struct {
    bool level;
    uint32_t duration_us;
} blusys_rmt_pulse_t;

blusys_err_t blusys_rmt_open(int pin, bool idle_level, blusys_rmt_t **out_rmt);
blusys_err_t blusys_rmt_close(blusys_rmt_t *rmt);
blusys_err_t blusys_rmt_write(blusys_rmt_t *rmt,
                              const blusys_rmt_pulse_t *pulses,
                              size_t pulse_count,
                              int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
