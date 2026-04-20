/**
 * @file counter.h
 * @brief Monotonic 32-bit counters, one slot per error domain.
 *
 * Cheap to bump from any context (atomic add). The diagnostics capability
 * reads snapshots; the runtime may reset counters across long tests.
 */

#ifndef BLUSYS_OBSERVE_COUNTER_H
#define BLUSYS_OBSERVE_COUNTER_H

#include <stdint.h>

#include "blusys/observe/error_domain.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Add @p by to the counter for @p domain.
 * @param domain  Which counter to bump.
 * @param by      Amount to add.
 */
void     blusys_counter_inc(blusys_err_domain_t domain, uint32_t by);

/** @brief Read the current value of a domain's counter. */
uint32_t blusys_counter_get(blusys_err_domain_t domain);

/** @brief Snapshot of every counter, captured atomically with respect to readers. */
typedef struct {
    uint32_t values[err_domain_count]; /**< One entry per domain, indexed by ::blusys_err_domain_t. */
} blusys_counter_snapshot_t;

/**
 * @brief Capture all counter values into @p out.
 * @param out  Destination snapshot.
 */
void     blusys_counter_snapshot(blusys_counter_snapshot_t *out);

/**
 * @brief Restore every counter from a prior snapshot.
 * @param in  Source snapshot.
 */
void     blusys_counter_restore(const blusys_counter_snapshot_t *in);

/** @brief Clear every counter to zero. */
void     blusys_counter_reset_all(void);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_OBSERVE_COUNTER_H */
