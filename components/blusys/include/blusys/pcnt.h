#ifndef BLUSYS_PCNT_H
#define BLUSYS_PCNT_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_pcnt blusys_pcnt_t;

typedef enum {
    BLUSYS_PCNT_EDGE_RISING = 0,
    BLUSYS_PCNT_EDGE_FALLING,
    BLUSYS_PCNT_EDGE_BOTH,
} blusys_pcnt_edge_t;

typedef bool (*blusys_pcnt_callback_t)(blusys_pcnt_t *pcnt, int watch_point, void *user_ctx);

blusys_err_t blusys_pcnt_open(int pin,
                              blusys_pcnt_edge_t edge,
                              uint32_t max_glitch_ns,
                              blusys_pcnt_t **out_pcnt);
blusys_err_t blusys_pcnt_close(blusys_pcnt_t *pcnt);
blusys_err_t blusys_pcnt_start(blusys_pcnt_t *pcnt);
blusys_err_t blusys_pcnt_stop(blusys_pcnt_t *pcnt);
blusys_err_t blusys_pcnt_clear_count(blusys_pcnt_t *pcnt);
blusys_err_t blusys_pcnt_get_count(blusys_pcnt_t *pcnt, int *out_count);
blusys_err_t blusys_pcnt_add_watch_point(blusys_pcnt_t *pcnt, int count);
blusys_err_t blusys_pcnt_remove_watch_point(blusys_pcnt_t *pcnt, int count);
blusys_err_t blusys_pcnt_set_callback(blusys_pcnt_t *pcnt,
                                      blusys_pcnt_callback_t callback,
                                      void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif
