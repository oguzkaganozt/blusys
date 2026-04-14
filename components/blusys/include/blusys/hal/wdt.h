#ifndef BLUSYS_WDT_H
#define BLUSYS_WDT_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

blusys_err_t blusys_wdt_init(uint32_t timeout_ms, bool panic_on_timeout);
blusys_err_t blusys_wdt_deinit(void);
blusys_err_t blusys_wdt_subscribe(void);
blusys_err_t blusys_wdt_unsubscribe(void);
blusys_err_t blusys_wdt_feed(void);

#ifdef __cplusplus
}
#endif

#endif
