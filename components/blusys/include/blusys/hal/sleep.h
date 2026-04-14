#ifndef BLUSYS_SLEEP_H
#define BLUSYS_SLEEP_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLUSYS_SLEEP_WAKEUP_UNDEFINED = 0,
    BLUSYS_SLEEP_WAKEUP_TIMER,
    BLUSYS_SLEEP_WAKEUP_EXT0,
    BLUSYS_SLEEP_WAKEUP_EXT1,
    BLUSYS_SLEEP_WAKEUP_TOUCHPAD,
    BLUSYS_SLEEP_WAKEUP_GPIO,
    BLUSYS_SLEEP_WAKEUP_UART,
    BLUSYS_SLEEP_WAKEUP_ULP,
} blusys_sleep_wakeup_t;

blusys_err_t          blusys_sleep_enable_timer_wakeup(uint64_t us);
blusys_err_t          blusys_sleep_enable_ulp_wakeup(void);
blusys_err_t          blusys_sleep_enter_light(void);
blusys_err_t          blusys_sleep_enter_deep(void);
blusys_sleep_wakeup_t blusys_sleep_get_wakeup_cause(void);

#ifdef __cplusplus
}
#endif

#endif
