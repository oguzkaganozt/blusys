#include "blusys/hal/system.h"

#include "esp_system.h"
#include "esp_timer.h"

static blusys_reset_reason_t blusys_system_map_reset_reason(esp_reset_reason_t reason)
{
    switch (reason) {
    case ESP_RST_POWERON:
        return BLUSYS_RESET_REASON_POWER_ON;
    case ESP_RST_EXT:
        return BLUSYS_RESET_REASON_EXTERNAL;
    case ESP_RST_SW:
        return BLUSYS_RESET_REASON_SOFTWARE;
    case ESP_RST_PANIC:
        return BLUSYS_RESET_REASON_PANIC;
    case ESP_RST_INT_WDT:
        return BLUSYS_RESET_REASON_INTERRUPT_WATCHDOG;
    case ESP_RST_TASK_WDT:
        return BLUSYS_RESET_REASON_TASK_WATCHDOG;
    case ESP_RST_WDT:
        return BLUSYS_RESET_REASON_WATCHDOG;
    case ESP_RST_DEEPSLEEP:
        return BLUSYS_RESET_REASON_DEEP_SLEEP;
    case ESP_RST_BROWNOUT:
        return BLUSYS_RESET_REASON_BROWNOUT;
    case ESP_RST_SDIO:
        return BLUSYS_RESET_REASON_SDIO;
    case ESP_RST_USB:
        return BLUSYS_RESET_REASON_USB;
    case ESP_RST_JTAG:
        return BLUSYS_RESET_REASON_JTAG;
    case ESP_RST_EFUSE:
        return BLUSYS_RESET_REASON_EFUSE;
    case ESP_RST_PWR_GLITCH:
        return BLUSYS_RESET_REASON_POWER_GLITCH;
    case ESP_RST_CPU_LOCKUP:
        return BLUSYS_RESET_REASON_CPU_LOCKUP;
    case ESP_RST_UNKNOWN:
    default:
        return BLUSYS_RESET_REASON_UNKNOWN;
    }
}

void blusys_system_restart(void)
{
    esp_restart();
}

blusys_err_t blusys_system_uptime_us(uint64_t *out_us)
{
    int64_t uptime_us;

    if (out_us == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    uptime_us = esp_timer_get_time();
    if (uptime_us < 0) {
        return BLUSYS_ERR_INTERNAL;
    }

    *out_us = (uint64_t) uptime_us;

    return BLUSYS_OK;
}

blusys_err_t blusys_system_reset_reason(blusys_reset_reason_t *out_reason)
{
    if (out_reason == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out_reason = blusys_system_map_reset_reason(esp_reset_reason());

    return BLUSYS_OK;
}

blusys_err_t blusys_system_free_heap_bytes(size_t *out_bytes)
{
    if (out_bytes == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out_bytes = (size_t) esp_get_free_heap_size();

    return BLUSYS_OK;
}

blusys_err_t blusys_system_minimum_free_heap_bytes(size_t *out_bytes)
{
    if (out_bytes == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out_bytes = (size_t) esp_get_minimum_free_heap_size();

    return BLUSYS_OK;
}
