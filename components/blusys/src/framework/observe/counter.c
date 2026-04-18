/* framework/observe/counter.c — monotonic u32 counter slab, one per domain.
 *
 * Atomic add via stdatomic so any task or ISR-handled deferred work can bump
 * a counter without coordinating with the reducer. Snapshot reads are
 * relaxed; perfect coherence across domains is not a goal.
 */

#include "blusys/framework/observe/counter.h"

#include <stddef.h>
#include <stdatomic.h>

static _Atomic uint32_t g_counters[err_domain_count];

void blusys_counter_inc(blusys_err_domain_t domain, uint32_t by)
{
    if ((unsigned)domain >= (unsigned)err_domain_count) {
        return;
    }
    atomic_fetch_add_explicit(&g_counters[domain], by, memory_order_relaxed);
}

uint32_t blusys_counter_get(blusys_err_domain_t domain)
{
    if ((unsigned)domain >= (unsigned)err_domain_count) {
        return 0;
    }
    return atomic_load_explicit(&g_counters[domain], memory_order_relaxed);
}

void blusys_counter_snapshot(blusys_counter_snapshot_t *out)
{
    if (out == NULL) {
        return;
    }
    for (int i = 0; i < (int)err_domain_count; ++i) {
        out->values[i] = atomic_load_explicit(&g_counters[i], memory_order_relaxed);
    }
}

void blusys_counter_restore(const blusys_counter_snapshot_t *in)
{
    if (in == NULL) {
        return;
    }
    for (int i = 0; i < (int)err_domain_count; ++i) {
        atomic_store_explicit(&g_counters[i], in->values[i], memory_order_relaxed);
    }
}

void blusys_counter_reset_all(void)
{
    for (int i = 0; i < (int)err_domain_count; ++i) {
        atomic_store_explicit(&g_counters[i], 0u, memory_order_relaxed);
    }
}
