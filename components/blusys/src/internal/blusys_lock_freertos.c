#include <stddef.h>

#include "blusys/internal/blusys_lock.h"

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

blusys_err_t blusys_lock_take(blusys_lock_t *lock, uint32_t timeout_ms)
{
    TickType_t timeout_ticks;

    if ((lock == NULL) || (lock->handle == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    timeout_ticks = (timeout_ms == BLUSYS_LOCK_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    return (xSemaphoreTake(lock->handle, timeout_ticks) == pdTRUE) ? BLUSYS_OK : BLUSYS_ERR_TIMEOUT;
}

blusys_err_t blusys_lock_give(blusys_lock_t *lock)
{
    if ((lock == NULL) || (lock->handle == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return (xSemaphoreGive(lock->handle) == pdTRUE) ? BLUSYS_OK : BLUSYS_ERR_INVALID_STATE;
}
