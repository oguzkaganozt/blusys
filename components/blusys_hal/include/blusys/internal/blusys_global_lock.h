#ifndef BLUSYS_GLOBAL_LOCK_H
#define BLUSYS_GLOBAL_LOCK_H

#include <stdbool.h>

#include "blusys/error.h"
#include "blusys/internal/blusys_lock.h"

/*
 * Shared "lazy, process-wide singleton lock" helper used by services whose
 * C-style callbacks (esp_wifi, esp_now, nimble, sntp, usb_host_hid) have no
 * user-data pointer and must therefore dereference a static handle.  Each
 * such module needs a lock that's initialised once, safely, across concurrent
 * open() calls.
 *
 * Usage (in a .c file only):
 *
 *     BLUSYS_DEFINE_GLOBAL_LOCK(hid)
 *
 * expands to:
 *
 *     static portMUX_TYPE  s_hid_init_lock = portMUX_INITIALIZER_UNLOCKED;
 *     static blusys_lock_t s_hid_global_lock;
 *     static bool          s_hid_global_lock_inited;
 *     static blusys_err_t ensure_global_lock(void) { ... }
 *
 * Callers refer to `s_<prefix>_global_lock` directly when taking/giving the
 * lock, and call `ensure_global_lock()` at the top of their public open()
 * path.  The double-checked init is a no-op once the flag is set.
 */
#define BLUSYS_DEFINE_GLOBAL_LOCK(prefix)                                     \
    static portMUX_TYPE  s_##prefix##_init_lock = portMUX_INITIALIZER_UNLOCKED; \
    static blusys_lock_t s_##prefix##_global_lock;                            \
    static bool          s_##prefix##_global_lock_inited;                     \
                                                                              \
    static blusys_err_t ensure_global_lock(void)                              \
    {                                                                         \
        if (s_##prefix##_global_lock_inited) {                                \
            return BLUSYS_OK;                                                 \
        }                                                                     \
        blusys_lock_t new_lock;                                               \
        blusys_err_t err = blusys_lock_init(&new_lock);                       \
        if (err != BLUSYS_OK) {                                               \
            return err;                                                       \
        }                                                                     \
        portENTER_CRITICAL(&s_##prefix##_init_lock);                          \
        if (!s_##prefix##_global_lock_inited) {                               \
            s_##prefix##_global_lock         = new_lock;                      \
            s_##prefix##_global_lock_inited  = true;                          \
            portEXIT_CRITICAL(&s_##prefix##_init_lock);                       \
        } else {                                                              \
            portEXIT_CRITICAL(&s_##prefix##_init_lock);                       \
            blusys_lock_deinit(&new_lock);                                    \
        }                                                                     \
        return BLUSYS_OK;                                                     \
    }                                                                         \
    struct blusys_global_lock_##prefix##_semicolon_swallower  /* no trailing ; */

#endif
