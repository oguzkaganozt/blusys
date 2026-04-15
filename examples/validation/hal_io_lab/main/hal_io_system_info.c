#include "sdkconfig.h"


#if CONFIG_HAL_IO_LAB_SCENARIO_SYSTEM_INFO

#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>

#include "blusys/blusys.h"

static const char *reset_reason_name(blusys_reset_reason_t reason)
{
    switch (reason) {
    case BLUSYS_RESET_REASON_POWER_ON:
        return "power_on";
    case BLUSYS_RESET_REASON_EXTERNAL:
        return "external";
    case BLUSYS_RESET_REASON_SOFTWARE:
        return "software";
    case BLUSYS_RESET_REASON_PANIC:
        return "panic";
    case BLUSYS_RESET_REASON_INTERRUPT_WATCHDOG:
        return "interrupt_watchdog";
    case BLUSYS_RESET_REASON_TASK_WATCHDOG:
        return "task_watchdog";
    case BLUSYS_RESET_REASON_WATCHDOG:
        return "watchdog";
    case BLUSYS_RESET_REASON_DEEP_SLEEP:
        return "deep_sleep";
    case BLUSYS_RESET_REASON_BROWNOUT:
        return "brownout";
    case BLUSYS_RESET_REASON_SDIO:
        return "sdio";
    case BLUSYS_RESET_REASON_USB:
        return "usb";
    case BLUSYS_RESET_REASON_JTAG:
        return "jtag";
    case BLUSYS_RESET_REASON_EFUSE:
        return "efuse";
    case BLUSYS_RESET_REASON_POWER_GLITCH:
        return "power_glitch";
    case BLUSYS_RESET_REASON_CPU_LOCKUP:
        return "cpu_lockup";
    case BLUSYS_RESET_REASON_UNKNOWN:
    default:
        return "unknown";
    }
}

static bool read_system_info(uint64_t *uptime_us,
                             blusys_reset_reason_t *reset_reason,
                             size_t *free_heap,
                             size_t *minimum_free_heap)
{
    blusys_err_t err;

    err = blusys_system_uptime_us(uptime_us);
    if (err != BLUSYS_OK) {
        printf("uptime error: %s\n", blusys_err_string(err));
        return false;
    }

    err = blusys_system_reset_reason(reset_reason);
    if (err != BLUSYS_OK) {
        printf("reset reason error: %s\n", blusys_err_string(err));
        return false;
    }

    err = blusys_system_free_heap_bytes(free_heap);
    if (err != BLUSYS_OK) {
        printf("free heap error: %s\n", blusys_err_string(err));
        return false;
    }

    err = blusys_system_minimum_free_heap_bytes(minimum_free_heap);
    if (err != BLUSYS_OK) {
        printf("minimum free heap error: %s\n", blusys_err_string(err));
        return false;
    }

    return true;
}

void run_system_info(void)
{
    uint64_t uptime_us;
    blusys_reset_reason_t reset_reason;
    size_t free_heap;
    size_t minimum_free_heap;

    if (!read_system_info(&uptime_us, &reset_reason, &free_heap, &minimum_free_heap)) {
        return;
    }

    printf("Blusys system info\n");
    printf("version: %s\n", blusys_version_string());
    printf("target: %s\n", blusys_target_name());
    printf("uptime_us: %" PRIu64 "\n", uptime_us);
    printf("reset_reason: %s\n", reset_reason_name(reset_reason));
    printf("free_heap_bytes: %zu\n", free_heap);
    printf("minimum_free_heap_bytes: %zu\n", minimum_free_heap);
}


#else

void run_system_info(void) {}

#endif /* CONFIG_HAL_IO_LAB_SCENARIO_SYSTEM_INFO */
