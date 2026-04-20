/* blusys/observe/trace.h — verbose tracing that compiles out.
 *
 *   BLUSYS_TRACE(domain, fmt, ...)
 *
 * Resolves to BLUSYS_LOG at the TRACE level under BLUSYS_OBSERVE_TRACE_ENABLED
 * (default: enabled in debug, disabled when NDEBUG is defined). When disabled
 * it expands to a no-op that still type-checks fmt arguments at -Wformat sites.
 */

#ifndef BLUSYS_OBSERVE_TRACE_H
#define BLUSYS_OBSERVE_TRACE_H

#include "blusys/observe/log.h"

#ifndef BLUSYS_OBSERVE_TRACE_ENABLED
#  ifdef NDEBUG
#    define BLUSYS_OBSERVE_TRACE_ENABLED 0
#  else
#    define BLUSYS_OBSERVE_TRACE_ENABLED 1
#  endif
#endif

#if BLUSYS_OBSERVE_TRACE_ENABLED
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
