#ifndef BLUSYS_CALLBACK_LIFECYCLE_H
#define BLUSYS_CALLBACK_LIFECYCLE_H

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Shared callback bookkeeping.
 *
 * Callers own synchronization. These helpers only centralize the counter,
 * closing flag, and drain loop so the same pattern is not reimplemented in
 * every HAL/driver source file.
 */

static inline bool blusys_callback_lifecycle_try_enter(volatile unsigned int *callback_active,
                                                       const volatile bool    *closing)
{
    if ((closing != NULL) && *closing) {
        return false;
    }

    *callback_active += 1u;
    return true;
}

static inline void blusys_callback_lifecycle_leave(volatile unsigned int *callback_active)
{
    if (*callback_active > 0u) {
        *callback_active -= 1u;
    }
}

static inline bool blusys_callback_lifecycle_is_active(const volatile unsigned int *callback_active)
{
    return *callback_active > 0u;
}

static inline void blusys_callback_lifecycle_wait_for_idle(const volatile unsigned int *callback_active)
{
    while (blusys_callback_lifecycle_is_active(callback_active)) {
        taskYIELD();
    }
}

static inline void blusys_callback_lifecycle_set_closing(volatile bool *closing, bool value)
{
    *closing = value;
}

static inline bool blusys_callback_lifecycle_is_closing(const volatile bool *closing)
{
    return (closing != NULL) && *closing;
}

#endif /* BLUSYS_CALLBACK_LIFECYCLE_H */
