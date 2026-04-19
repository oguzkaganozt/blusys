// Headless platform helpers on ESP-IDF (BLUSYS_APP_HEADLESS).
//
// Mirrors scripts/host/src/app_headless_platform.cpp for desktop.

#if BLUSYS_DEVICE_BUILD

#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdint>

namespace blusys::platform {

std::uint32_t headless_get_ticks_ms()
{
    return static_cast<std::uint32_t>(esp_timer_get_time() / 1000ULL);
}

void headless_delay(std::uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

}  // namespace blusys::platform

#endif  // BLUSYS_DEVICE_BUILD
