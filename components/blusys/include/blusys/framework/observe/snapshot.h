#ifndef BLUSYS_FRAMEWORK_OBSERVE_SNAPSHOT_H
#define BLUSYS_FRAMEWORK_OBSERVE_SNAPSHOT_H

#include <stdbool.h>
#include <stddef.h>

#include "blusys/framework/observe/counter.h"
#include "blusys/framework/observe/log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLUSYS_OBSERVE_LOG_RING_DEPTH 32u
#define BLUSYS_OBSERVE_LINE_MAX        160u

typedef struct {
    uint32_t            sequence;
    blusys_log_level_t  level;
    blusys_err_domain_t domain;
    char                text[BLUSYS_OBSERVE_LINE_MAX];
} blusys_observe_log_entry_t;

typedef struct {
    uint32_t            sequence;
    blusys_log_level_t  level;
    blusys_err_domain_t domain;
    bool                valid;
    char                text[BLUSYS_OBSERVE_LINE_MAX];
} blusys_observe_last_error_t;

typedef struct {
    uint32_t                     log_count;
    blusys_observe_log_entry_t   logs[BLUSYS_OBSERVE_LOG_RING_DEPTH];
    blusys_counter_snapshot_t    counters;
    blusys_observe_last_error_t  last_errors[err_domain_count];
} blusys_observe_snapshot_t;

void blusys_observe_record_log(blusys_log_level_t level,
                               blusys_err_domain_t domain,
                               const char *text);
void blusys_observe_snapshot(blusys_observe_snapshot_t *out);
void blusys_observe_restore(const blusys_observe_snapshot_t *in);
void blusys_observe_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_FRAMEWORK_OBSERVE_SNAPSHOT_H */
