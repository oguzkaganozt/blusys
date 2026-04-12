#ifndef BLUSYS_SYSTEM_H
#define BLUSYS_SYSTEM_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLUSYS_RESET_REASON_UNKNOWN = 0,
    BLUSYS_RESET_REASON_POWER_ON,
    BLUSYS_RESET_REASON_EXTERNAL,
    BLUSYS_RESET_REASON_SOFTWARE,
    BLUSYS_RESET_REASON_PANIC,
    BLUSYS_RESET_REASON_INTERRUPT_WATCHDOG,
    BLUSYS_RESET_REASON_TASK_WATCHDOG,
    BLUSYS_RESET_REASON_WATCHDOG,
    BLUSYS_RESET_REASON_DEEP_SLEEP,
    BLUSYS_RESET_REASON_BROWNOUT,
    BLUSYS_RESET_REASON_SDIO,
    BLUSYS_RESET_REASON_USB,
    BLUSYS_RESET_REASON_JTAG,
    BLUSYS_RESET_REASON_EFUSE,
    BLUSYS_RESET_REASON_POWER_GLITCH,
    BLUSYS_RESET_REASON_CPU_LOCKUP,
} blusys_reset_reason_t;

void blusys_system_restart(void);
blusys_err_t blusys_system_uptime_us(uint64_t *out_us);
blusys_err_t blusys_system_reset_reason(blusys_reset_reason_t *out_reason);
blusys_err_t blusys_system_free_heap_bytes(size_t *out_bytes);
blusys_err_t blusys_system_minimum_free_heap_bytes(size_t *out_bytes);

#ifdef __cplusplus
}
#endif

#endif
