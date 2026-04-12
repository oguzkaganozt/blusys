#include "blusys/wdt.h"

#include "blusys/internal/blusys_esp_err.h"

#include "esp_task_wdt.h"

blusys_err_t blusys_wdt_init(uint32_t timeout_ms, bool panic_on_timeout)
{
    esp_task_wdt_config_t cfg = {
        .timeout_ms     = timeout_ms,
        .idle_core_mask = 0,
        .trigger_panic  = panic_on_timeout,
    };
    esp_err_t esp_err = esp_task_wdt_init(&cfg);
    if (esp_err == ESP_ERR_INVALID_STATE) {
        esp_err = esp_task_wdt_reconfigure(&cfg);
    }
    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_wdt_deinit(void)
{
    return blusys_translate_esp_err(esp_task_wdt_deinit());
}

blusys_err_t blusys_wdt_subscribe(void)
{
    return blusys_translate_esp_err(esp_task_wdt_add(NULL));
}

blusys_err_t blusys_wdt_unsubscribe(void)
{
    return blusys_translate_esp_err(esp_task_wdt_delete(NULL));
}

blusys_err_t blusys_wdt_feed(void)
{
    return blusys_translate_esp_err(esp_task_wdt_reset());
}
