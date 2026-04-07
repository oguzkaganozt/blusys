#include "blusys/sleep.h"

#include "blusys/internal/blusys_esp_err.h"

#include "esp_sleep.h"

blusys_err_t blusys_sleep_enable_timer_wakeup(uint64_t us)
{
    return blusys_translate_esp_err(esp_sleep_enable_timer_wakeup(us));
}

blusys_err_t blusys_sleep_enter_light(void)
{
    return blusys_translate_esp_err(esp_light_sleep_start());
}

blusys_err_t blusys_sleep_enter_deep(void)
{
    esp_deep_sleep_start();
    return BLUSYS_OK;
}

blusys_sleep_wakeup_t blusys_sleep_get_wakeup_cause(void)
{
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:    return BLUSYS_SLEEP_WAKEUP_TIMER;
        case ESP_SLEEP_WAKEUP_EXT0:     return BLUSYS_SLEEP_WAKEUP_EXT0;
        case ESP_SLEEP_WAKEUP_EXT1:     return BLUSYS_SLEEP_WAKEUP_EXT1;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: return BLUSYS_SLEEP_WAKEUP_TOUCHPAD;
        case ESP_SLEEP_WAKEUP_GPIO:     return BLUSYS_SLEEP_WAKEUP_GPIO;
        case ESP_SLEEP_WAKEUP_UART:     return BLUSYS_SLEEP_WAKEUP_UART;
        default:                        return BLUSYS_SLEEP_WAKEUP_UNDEFINED;
    }
}
