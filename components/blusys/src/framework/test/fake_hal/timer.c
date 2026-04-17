/* src/framework/test/fake_hal/timer.c — virtual timer wheel for tests.
 *
 * Tests drive time by calling blusys_test_timer_advance_us(). Every periodic
 * timer fires for each multiple of its period that fits inside the advance
 * window; one-shot timers fire once and then stop. Callbacks run on the
 * caller's thread, which in tests is the reducer thread. That matches the
 * ISR→task handoff contract documented in docs/internals/threading.md: the
 * callback must not touch app state directly — capabilities hand results to
 * the reducer via post_event().
 */

#include "blusys/framework/test/fake_hal.h"
#include "blusys/hal/timer.h"

#include <stdlib.h>
#include <string.h>

struct blusys_timer {
    uint32_t                 period_us;
    bool                     periodic;
    bool                     started;
    blusys_timer_callback_t  cb;
    void                    *cb_ctx;
    uint64_t                 next_fire_us;
    bool                     alive;
};

#define FAKE_TIMER_MAX 32

static struct blusys_timer g_timers[FAKE_TIMER_MAX];
static uint64_t            g_now_us = 0;

static struct blusys_timer *alloc_slot(void)
{
    for (int i = 0; i < FAKE_TIMER_MAX; ++i) {
        if (!g_timers[i].alive) {
            memset(&g_timers[i], 0, sizeof(g_timers[i]));
            g_timers[i].alive = true;
            return &g_timers[i];
        }
    }
    return NULL;
}

blusys_err_t blusys_timer_open(uint32_t period_us, bool periodic, blusys_timer_t **out_timer)
{
    if (out_timer == NULL || period_us == 0) return BLUSYS_ERR_INVALID_ARG;
    blusys_timer_t *t = alloc_slot();
    if (t == NULL) return BLUSYS_ERR_NO_MEM;
    t->period_us = period_us;
    t->periodic  = periodic;
    *out_timer   = t;
    return BLUSYS_OK;
}

blusys_err_t blusys_timer_close(blusys_timer_t *timer)
{
    if (timer == NULL) return BLUSYS_ERR_INVALID_ARG;
    timer->alive   = false;
    timer->started = false;
    return BLUSYS_OK;
}

blusys_err_t blusys_timer_start(blusys_timer_t *timer)
{
    if (timer == NULL) return BLUSYS_ERR_INVALID_ARG;
    timer->started      = true;
    timer->next_fire_us = g_now_us + timer->period_us;
    return BLUSYS_OK;
}

blusys_err_t blusys_timer_stop(blusys_timer_t *timer)
{
    if (timer == NULL) return BLUSYS_ERR_INVALID_ARG;
    timer->started = false;
    return BLUSYS_OK;
}

blusys_err_t blusys_timer_set_period(blusys_timer_t *timer, uint32_t period_us)
{
    if (timer == NULL || period_us == 0) return BLUSYS_ERR_INVALID_ARG;
    timer->period_us = period_us;
    return BLUSYS_OK;
}

blusys_err_t blusys_timer_set_callback(blusys_timer_t *timer,
                                       blusys_timer_callback_t cb,
                                       void *user_ctx)
{
    if (timer == NULL) return BLUSYS_ERR_INVALID_ARG;
    timer->cb     = cb;
    timer->cb_ctx = user_ctx;
    return BLUSYS_OK;
}

/* ── test inject API ──────────────────────────────────────────────────── */

size_t blusys_test_timer_advance_us(uint64_t us)
{
    uint64_t target = g_now_us + us;
    size_t   hits   = 0;

    /* Fire in time order. Cheapest correct version: loop while any timer is
     * due. Max FAKE_TIMER_MAX iterations per fire, which is fine for tests. */
    for (;;) {
        struct blusys_timer *soonest = NULL;
        for (int i = 0; i < FAKE_TIMER_MAX; ++i) {
            struct blusys_timer *t = &g_timers[i];
            if (!t->alive || !t->started) continue;
            if (t->next_fire_us > target)  continue;
            if (soonest == NULL || t->next_fire_us < soonest->next_fire_us) {
                soonest = t;
            }
        }
        if (soonest == NULL) break;

        g_now_us = soonest->next_fire_us;
        if (soonest->cb) {
            soonest->cb(soonest, soonest->cb_ctx);
            ++hits;
        }
        if (soonest->periodic) {
            soonest->next_fire_us += soonest->period_us;
        } else {
            soonest->started = false;
        }
    }
    g_now_us = target;
    return hits;
}

void blusys_test_timer_reset_all(void)
{
    memset(g_timers, 0, sizeof(g_timers));
    g_now_us = 0;
}
