#ifndef BLUSYS_TIMER_H
#define BLUSYS_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_timer blusys_timer_t;

typedef bool (*blusys_timer_callback_t)(blusys_timer_t *timer, void *user_ctx);

blusys_err_t blusys_timer_open(uint32_t period_us, bool periodic, blusys_timer_t **out_timer);
blusys_err_t blusys_timer_close(blusys_timer_t *timer);
blusys_err_t blusys_timer_start(blusys_timer_t *timer);
blusys_err_t blusys_timer_stop(blusys_timer_t *timer);
blusys_err_t blusys_timer_set_period(blusys_timer_t *timer, uint32_t period_us);
blusys_err_t blusys_timer_set_callback(blusys_timer_t *timer,
                                       blusys_timer_callback_t callback,
                                       void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif
