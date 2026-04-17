/* blusys/framework/observe/log.h — structured log front-end.
 *
 * One macro, one signature, one output format on host and device:
 *
 *   BLUSYS_LOG(level, domain, fmt, ...)
 *
 * Lines are emitted as "[L][domain.name] message\n" via a shared printf-backed
 * sink so that diagnostics tooling can parse the same shape regardless of
 * platform. Level filtering happens once in the sink.
 *
 * Existing tag-based BLUSYS_LOG{E,W,I,D} macros in blusys/hal/log.h remain for
 * subsystems that have not migrated yet. New code should use this header.
 */

#ifndef BLUSYS_FRAMEWORK_OBSERVE_LOG_H
#define BLUSYS_FRAMEWORK_OBSERVE_LOG_H

#include <stdarg.h>

#include "blusys/framework/observe/error_domain.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLUSYS_LOG_ERROR = 0,
    BLUSYS_LOG_WARN,
    BLUSYS_LOG_INFO,
    BLUSYS_LOG_DEBUG,
    BLUSYS_LOG_TRACE,
} blusys_log_level_t;

void blusys_log(blusys_log_level_t       level,
                blusys_err_domain_t      domain,
                const char              *fmt,
                ...) __attribute__((format(printf, 3, 4)));

void blusys_log_v(blusys_log_level_t    level,
                  blusys_err_domain_t   domain,
                  const char           *fmt,
                  va_list               ap);

/* Runtime threshold; lines below `min` are dropped. Default: BLUSYS_LOG_INFO. */
void               blusys_log_set_min_level(blusys_log_level_t min);
blusys_log_level_t blusys_log_get_min_level(void);

#define BLUSYS_LOG(level, domain, fmt, ...) \
    blusys_log((level), (domain), (fmt), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_FRAMEWORK_OBSERVE_LOG_H */
