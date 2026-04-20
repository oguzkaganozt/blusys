#include "blusys/observe/snapshot.h"

#include <stddef.h>
#include <string.h>

static blusys_observe_log_entry_t  g_log_ring[BLUSYS_OBSERVE_LOG_RING_DEPTH];
static blusys_observe_last_error_t g_last_errors[err_domain_count];
static uint32_t                    g_log_sequence = 0;
static uint32_t                    g_log_count     = 0;

static unsigned int ring_index(uint32_t sequence)
{
    return (unsigned int)(sequence % BLUSYS_OBSERVE_LOG_RING_DEPTH);
}

void blusys_observe_record_log(blusys_log_level_t level,
                               blusys_err_domain_t domain,
                               const char *text)
{
    if ((unsigned)domain >= (unsigned)err_domain_count || text == NULL) {
        return;
    }

    const uint32_t seq = ++g_log_sequence;
    blusys_observe_log_entry_t *entry = &g_log_ring[ring_index(seq - 1u)];
    entry->sequence = seq;
    entry->level    = level;
    entry->domain   = domain;
    strncpy(entry->text, text, sizeof(entry->text) - 1u);
    entry->text[sizeof(entry->text) - 1u] = '\0';

    if (g_log_count < BLUSYS_OBSERVE_LOG_RING_DEPTH) {
        ++g_log_count;
    }

    if (level == BLUSYS_LOG_ERROR) {
        blusys_observe_last_error_t *last = &g_last_errors[domain];
        last->sequence = seq;
        last->level    = level;
        last->domain   = domain;
        last->valid    = true;
        strncpy(last->text, text, sizeof(last->text) - 1u);
        last->text[sizeof(last->text) - 1u] = '\0';
    }
}

void blusys_observe_snapshot(blusys_observe_snapshot_t *out)
{
    if (out == NULL) {
        return;
    }

    memset(out, 0, sizeof(*out));
    out->log_count = g_log_count;

    const uint32_t count = g_log_count < BLUSYS_OBSERVE_LOG_RING_DEPTH
                               ? g_log_count
                               : BLUSYS_OBSERVE_LOG_RING_DEPTH;
    const uint32_t start = g_log_sequence > count ? g_log_sequence - count : 0u;
    for (uint32_t i = 0; i < count; ++i) {
        blusys_observe_log_entry_t *dst = &out->logs[i];
        const blusys_observe_log_entry_t *src = &g_log_ring[ring_index(start + i)];
        *dst = *src;
    }

    blusys_counter_snapshot(&out->counters);
    for (int i = 0; i < (int)err_domain_count; ++i) {
        out->last_errors[i] = g_last_errors[i];
    }
}

void blusys_observe_restore(const blusys_observe_snapshot_t *in)
{
    if (in == NULL) {
        return;
    }

    memset(g_log_ring, 0, sizeof(g_log_ring));
    memset(g_last_errors, 0, sizeof(g_last_errors));
    g_log_sequence = 0;
    g_log_count = 0;

    const uint32_t count = in->log_count < BLUSYS_OBSERVE_LOG_RING_DEPTH
                               ? in->log_count
                               : BLUSYS_OBSERVE_LOG_RING_DEPTH;
    for (uint32_t i = 0; i < count; ++i) {
        const blusys_observe_log_entry_t *src = &in->logs[i];
        if (src->sequence == 0u) {
            continue;
        }
        blusys_observe_log_entry_t *dst = &g_log_ring[ring_index(src->sequence - 1u)];
        *dst = *src;
        if (src->sequence > g_log_sequence) {
            g_log_sequence = src->sequence;
        }
        ++g_log_count;
    }

    for (int i = 0; i < (int)err_domain_count; ++i) {
        g_last_errors[i] = in->last_errors[i];
    }

    blusys_counter_restore(&in->counters);
}

void blusys_observe_clear(void)
{
    memset(g_log_ring, 0, sizeof(g_log_ring));
    memset(g_last_errors, 0, sizeof(g_last_errors));
    g_log_sequence = 0;
    g_log_count = 0;
    blusys_counter_reset_all();
}
