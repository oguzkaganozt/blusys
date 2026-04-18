/* framework/observe/log.c — printf-backed structured log sink.
 *
 * Single output format on host and device:
 *
 *   [L][domain.name] message\n
 *
 * Goes to stdout for INFO/DEBUG/TRACE and stderr for ERROR/WARN. ESP-IDF maps
 * stdout/stderr onto the same UART by default, so device output remains usable
 * without dragging in esp_log's own format. Host builds get the same shape.
 */

#include "blusys/framework/observe/log.h"
#include "blusys/framework/observe/snapshot.h"

#include <stdio.h>
#include <string.h>

static blusys_log_level_t g_min_level = BLUSYS_LOG_INFO;

static char level_letter(blusys_log_level_t level)
{
    switch (level) {
    case BLUSYS_LOG_ERROR: return 'E';
    case BLUSYS_LOG_WARN:  return 'W';
    case BLUSYS_LOG_INFO:  return 'I';
    case BLUSYS_LOG_DEBUG: return 'D';
    case BLUSYS_LOG_TRACE: return 'T';
    }
    return '?';
}

void blusys_log_set_min_level(blusys_log_level_t min)
{
    g_min_level = min;
}

blusys_log_level_t blusys_log_get_min_level(void)
{
    return g_min_level;
}

void blusys_log_v(blusys_log_level_t  level,
                  blusys_err_domain_t domain,
                  const char         *fmt,
                  va_list             ap)
{
    if (level > g_min_level) {
        return;
    }
    char message[BLUSYS_OBSERVE_LINE_MAX] = {0};
    char rendered[BLUSYS_OBSERVE_LINE_MAX + 16] = {0};
    vsnprintf(message, sizeof(message), fmt, ap);
    snprintf(rendered, sizeof(rendered), "[%c][%s] %s",
             level_letter(level), blusys_err_domain_string(domain), message);

    FILE *out = (level <= BLUSYS_LOG_WARN) ? stderr : stdout;
    fputs(rendered, out);
    fputc('\n', out);

    blusys_counter_inc(domain, 1u);
    blusys_observe_record_log(level, domain, rendered);
}

void blusys_log(blusys_log_level_t  level,
                blusys_err_domain_t domain,
                const char         *fmt,
                ...)
{
    va_list ap;
    va_start(ap, fmt);
    blusys_log_v(level, domain, fmt, ap);
    va_end(ap);
}
