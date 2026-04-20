/**
 * @file snapshot.h
 * @brief Ring-buffered log capture and snapshot of counters + last-errors.
 *
 * The diagnostics capability uses these snapshots to surface recent log lines,
 * domain counters, and the most recent error per domain without allocating.
 */

#ifndef BLUSYS_OBSERVE_SNAPSHOT_H
#define BLUSYS_OBSERVE_SNAPSHOT_H

#include <stdbool.h>
#include <stddef.h>

#include "blusys/observe/counter.h"
#include "blusys/observe/log.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Depth of the captured log ring buffer. */
#define BLUSYS_OBSERVE_LOG_RING_DEPTH 32u

/** @brief Maximum rendered length of a single captured log line (bytes). */
#define BLUSYS_OBSERVE_LINE_MAX        160u

/** @brief One entry in the captured log ring. */
typedef struct {
    uint32_t            sequence;                       /**< Monotonic sequence number. */
    blusys_log_level_t  level;                          /**< Severity. */
    blusys_err_domain_t domain;                         /**< Originating domain. */
    char                text[BLUSYS_OBSERVE_LINE_MAX];  /**< Rendered line, null-terminated. */
} blusys_observe_log_entry_t;

/** @brief Most recent error line observed for a given domain. */
typedef struct {
    uint32_t            sequence;                       /**< Sequence number at capture. */
    blusys_log_level_t  level;                          /**< Severity (always error when @p valid). */
    blusys_err_domain_t domain;                         /**< Originating domain. */
    bool                valid;                          /**< `true` if an error has been recorded for this domain. */
    char                text[BLUSYS_OBSERVE_LINE_MAX];  /**< Rendered line, null-terminated. */
} blusys_observe_last_error_t;

/** @brief Atomic snapshot of observe state: recent logs, counters, last errors. */
typedef struct {
    uint32_t                     log_count;                            /**< Number of valid entries in @p logs (≤ ::BLUSYS_OBSERVE_LOG_RING_DEPTH). */
    blusys_observe_log_entry_t   logs[BLUSYS_OBSERVE_LOG_RING_DEPTH];  /**< Captured log ring. */
    blusys_counter_snapshot_t    counters;                             /**< Domain counter snapshot. */
    blusys_observe_last_error_t  last_errors[err_domain_count];        /**< Last error per domain. */
} blusys_observe_snapshot_t;

/**
 * @brief Record a log line into the ring and (if error-level) the last-error slot for @p domain.
 * @param level   Severity.
 * @param domain  Originating subsystem domain.
 * @param text    Rendered line, null-terminated.
 */
void blusys_observe_record_log(blusys_log_level_t level,
                               blusys_err_domain_t domain,
                               const char *text);

/** @brief Capture the current observe state into @p out. */
void blusys_observe_snapshot(blusys_observe_snapshot_t *out);

/** @brief Restore observe state from a prior snapshot @p in. */
void blusys_observe_restore(const blusys_observe_snapshot_t *in);

/** @brief Clear the log ring, counters, and last-error slots. */
void blusys_observe_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_OBSERVE_SNAPSHOT_H */
