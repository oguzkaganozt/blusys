#include "blusys/internal/blusys_bt_stack.h"

#include <stdbool.h>

#include "freertos/FreeRTOS.h"

#include "blusys/internal/blusys_lock.h"

static portMUX_TYPE            s_bt_stack_init_lock = portMUX_INITIALIZER_UNLOCKED;
static blusys_lock_t           s_bt_stack_lock;
static bool                    s_bt_stack_lock_inited;
static blusys_bt_stack_owner_t s_bt_stack_owner;
static bool                    s_bt_stack_owned;

static blusys_err_t ensure_bt_stack_lock(void)
{
    if (s_bt_stack_lock_inited) {
        return BLUSYS_OK;
    }

    blusys_lock_t new_lock;
    blusys_err_t err = blusys_lock_init(&new_lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    portENTER_CRITICAL(&s_bt_stack_init_lock);
    if (!s_bt_stack_lock_inited) {
        s_bt_stack_lock = new_lock;
        s_bt_stack_lock_inited = true;
        portEXIT_CRITICAL(&s_bt_stack_init_lock);
    } else {
        portEXIT_CRITICAL(&s_bt_stack_init_lock);
        blusys_lock_deinit(&new_lock);
    }

    return BLUSYS_OK;
}

blusys_err_t blusys_bt_stack_acquire(blusys_bt_stack_owner_t owner)
{
    blusys_err_t err = ensure_bt_stack_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_bt_stack_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (s_bt_stack_owned) {
        blusys_lock_give(&s_bt_stack_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    s_bt_stack_owner = owner;
    s_bt_stack_owned = true;
    blusys_lock_give(&s_bt_stack_lock);
    return BLUSYS_OK;
}

void blusys_bt_stack_release(blusys_bt_stack_owner_t owner)
{
    if (!s_bt_stack_lock_inited) {
        return;
    }

    if (blusys_lock_take(&s_bt_stack_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return;
    }

    if (s_bt_stack_owned && s_bt_stack_owner == owner) {
        s_bt_stack_owned = false;
    }

    blusys_lock_give(&s_bt_stack_lock);
}
