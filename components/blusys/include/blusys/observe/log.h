/**
 * @file log.h
 * @brief Structured log front-end — one signature, one output format on host and device.
 *
 * Emit with `BLUSYS_LOG(level, domain, fmt, ...)`. Lines are formatted as
 * `"[L][domain.name] message\n"` via a shared printf-backed sink so diagnostics
 * tooling can parse the same shape regardless of platform. Level filtering
 * happens once in the sink.
 *
 * The older tag-based `BLUSYS_LOG{E,W,I,D}` macros in blusys/hal/log.h remain
 * for subsystems that have not migrated yet. New code should use this header.
 */

#ifndef BLUSYS_OBSERVE_LOG_H
#define BLUSYS_OBSERVE_LOG_H

#include <stdarg.h>

#include "blusys/observe/error_domain.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Severity level, ordered from most to least severe. */
typedef enum {
    BLUSYS_LOG_ERROR = 0,   /**< Unrecoverable or unexpected failure. */
    BLUSYS_LOG_WARN,        /**< Recoverable anomaly worth surfacing. */
    BLUSYS_LOG_INFO,        /**< Normal operational event. */
    BLUSYS_LOG_DEBUG,       /**< Developer-facing diagnostic detail. */
    BLUSYS_LOG_TRACE,       /**< Verbose tracing (see blusys/observe/trace.h). */
} blusys_log_level_t;

/**
 * @brief Emit a structured log line with printf-style formatting.
 *
 * @param level   Severity.
 * @param domain  Originating subsystem domain.
 * @param fmt     printf-style format string.
 */
void blusys_log(blusys_log_level_t       level,
                blusys_err_domain_t      domain,
                const char              *fmt,
                ...) __attribute__((format(printf, 3, 4)));

/**
 * @brief `va_list` variant of ::blusys_log for forwarding from wrapper functions.
 *
 * @param level   Severity.
 * @param domain  Originating subsystem domain.
 * @param fmt     printf-style format string.
 * @param ap      Argument list.
 */
void blusys_log_v(blusys_log_level_t    level,
                  blusys_err_domain_t   domain,
                  const char           *fmt,
                  va_list               ap);

/**
 * @brief Set the runtime severity threshold.
 *
 * Lines below @p min are dropped in the sink. Default is `BLUSYS_LOG_INFO`.
 *
 * @param min  Minimum severity to emit.
 */
void               blusys_log_set_min_level(blusys_log_level_t min);

/** @brief Return the current runtime severity threshold. */
blusys_log_level_t blusys_log_get_min_level(void);

/**
 * @brief Emit a structured log line.
 * @param level   Severity (::blusys_log_level_t).
 * @param domain  Originating subsystem domain (::blusys_err_domain_t).
 * @param fmt     printf-style format string.
 */
#define BLUSYS_LOG(level, domain, fmt, ...) \
    blusys_log((level), (domain), (fmt), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_OBSERVE_LOG_H */
