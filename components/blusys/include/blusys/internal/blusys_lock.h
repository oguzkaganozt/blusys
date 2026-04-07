#ifndef BLUSYS_LOCK_H
#define BLUSYS_LOCK_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "blusys/error.h"

#define BLUSYS_LOCK_WAIT_FOREVER UINT32_MAX

typedef struct {
    SemaphoreHandle_t handle;
} blusys_lock_t;

blusys_err_t blusys_lock_init(blusys_lock_t *lock);
void blusys_lock_deinit(blusys_lock_t *lock);
blusys_err_t blusys_lock_take(blusys_lock_t *lock, uint32_t timeout_ms);
blusys_err_t blusys_lock_give(blusys_lock_t *lock);

#endif
