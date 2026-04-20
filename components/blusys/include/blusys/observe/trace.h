/**
 * @file trace.h
 * @brief Verbose tracing that compiles out in release builds.
 *
 * `BLUSYS_TRACE(domain, fmt, ...)` resolves to a `BLUSYS_LOG_TRACE` call when
 * `BLUSYS_OBSERVE_TRACE_ENABLED` is non-zero (default: enabled in debug,
 * disabled when `NDEBUG` is defined). When disabled it expands to a no-op that
 * still type-checks @p fmt arguments at `-Wformat` sites.
 */

#ifndef BLUSYS_OBSERVE_TRACE_H
#define BLUSYS_OBSERVE_TRACE_H

#include "blusys/observe/log.h"

/** @brief Compile-time switch controlling whether ::BLUSYS_TRACE emits anything. */
#ifndef BLUSYS_OBSERVE_TRACE_ENABLED
#  ifdef NDEBUG
#    define BLUSYS_OBSERVE_TRACE_ENABLED 0
#  else
#    define BLUSYS_OBSERVE_TRACE_ENABLED 1
#  endif
#endif

#if BLUSYS_OBSERVE_TRACE_ENABLED
/**
 * @brief Emit a trace-level log line.
 * @param domain  Originating subsystem domain (::blusys_err_domain_t).
 * @param fmt     printf-style format string.
 */
#  define BLUSYS_TRACE(domain, fmt, ...) \
       blusys_log(BLUSYS_LOG_TRACE, (domain), (fmt), ##__VA_ARGS__)
#else
   /* Compile out, but still parse the format string for warnings. */
#  define BLUSYS_TRACE(domain, fmt, ...) \
       do {                              \
           (void)(domain);               \
           if (0) { (void)(fmt); }       \
       } while (0)
#endif

#endif /* BLUSYS_OBSERVE_TRACE_H */
