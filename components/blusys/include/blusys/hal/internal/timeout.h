#ifndef BLUSYS_TIMEOUT_H
#define BLUSYS_TIMEOUT_H

#include <stdbool.h>

#include "freertos/FreeRTOS.h"

#include "blusys/hal/error.h"

static inline bool blusys_timeout_ms_is_valid(int timeout_ms)
{
    return timeout_ms >= BLUSYS_TIMEOUT_FOREVER;
}

static inline TickType_t blusys_timeout_ms_to_ticks(int timeout_ms)
{
    return (timeout_ms == BLUSYS_TIMEOUT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
}

#endif
