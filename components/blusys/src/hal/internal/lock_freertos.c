#include <stddef.h>

#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/timeout.h"

blusys_err_t blusys_lock_init(blusys_lock_t *lock)
{
    if (lock == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    lock->handle = xSemaphoreCreateMutex();
    if (lock->handle == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    return BLUSYS_OK;
}

void blusys_lock_deinit(blusys_lock_t *lock)
{
    if ((lock == NULL) || (lock->handle == NULL)) {
        return;
    }

    vSemaphoreDelete(lock->handle);
    lock->handle = NULL;
}

blusys_err_t blusys_lock_take(blusys_lock_t *lock, int timeout_ms)
{
    if ((lock == NULL) || (lock->handle == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    TickType_t timeout_ticks = blusys_timeout_ms_to_ticks(timeout_ms);

    return (xSemaphoreTake(lock->handle, timeout_ticks) == pdTRUE) ? BLUSYS_OK : BLUSYS_ERR_TIMEOUT;
}

blusys_err_t blusys_lock_give(blusys_lock_t *lock)
{
    if ((lock == NULL) || (lock->handle == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return (xSemaphoreGive(lock->handle) == pdTRUE) ? BLUSYS_OK : BLUSYS_ERR_INVALID_STATE;
}
