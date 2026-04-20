/* blusys/framework/observe/counter.h — monotonic u32 counters per domain.
 *
 * One slot per blusys_err_domain_t. Cheap to bump from any context (atomic
 * add). The diagnostics capability reads snapshots; the runtime may reset
 * them across long tests.
 */

#ifndef BLUSYS_FRAMEWORK_OBSERVE_COUNTER_H
#define BLUSYS_FRAMEWORK_OBSERVE_COUNTER_H

#include <stdint.h>

#include "blusys/framework/observe/error_domain.h"

#ifdef __cplusplus
extern "C" {
#endif

void     blusys_counter_inc(blusys_err_domain_t domain, uint32_t by);
uint32_t blusys_counter_get(blusys_err_domain_t domain);
typedef struct {
    uint32_t values[err_domain_count];
} blusys_counter_snapshot_t;
void     blusys_counter_snapshot(blusys_counter_snapshot_t *out);
void     blusys_counter_restore(const blusys_counter_snapshot_t *in);
void     blusys_counter_reset_all(void);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_FRAMEWORK_OBSERVE_COUNTER_H */
