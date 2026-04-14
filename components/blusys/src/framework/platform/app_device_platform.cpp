// Device platform helpers for blusys::app device entry.
//
// These functions are called by the template run_device() in entry.hpp.
// They isolate all ESP-IDF, LCD, and UI service initialization from
// product code. Analogous to app_host_platform.cpp for the host path.
//
// This file only compiles in the IDF component build (it uses
// esp_timer.h and freertos/FreeRTOS.h).

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/platform/profile.hpp"
#include "blusys/framework/ui/style/presets.hpp"
#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/hal/log.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lvgl.h"

#include <cstdint>

namespace {
void *find_first_pointer_indev()
{
    lv_indev_t *indev = lv_indev_get_next(nullptr);
    while (indev != nullptr) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            return indev;
        }
        indev = lv_indev_get_next(indev);
    }
    return nullptr;
}
}  // namespace

namespace {
constexpr const char *kTag = "device_platform";
}

namespace blusys::platform {

blusys_err_t device_init(blusys::device_profile &profile,
                         blusys_lcd_t **lcd_out,
                         blusys_display_t **ui_out)
{
    if (lcd_out == nullptr || ui_out == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    // 1. Open LCD.
    blusys_err_t err = blusys_lcd_open(&profile.lcd, lcd_out);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "lcd open failed: %d", static_cast<int>(err));
        return err;
    }

    // 2. Wire LCD handle into UI config.
    profile.ui.lcd = *lcd_out;

    // 3. Open UI service (initializes LVGL, creates render task).
    err = blusys_display_open(&profile.ui, ui_out);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "ui open failed: %d", static_cast<int>(err));
        blusys_lcd_close(*lcd_out);
        *lcd_out = nullptr;
        return err;
    }

    // 4. Set initial backlight brightness.
    if (profile.brightness >= 0) {
        blusys_lcd_set_brightness(*lcd_out, profile.brightness);
    }

    BLUSYS_LOGI(kTag, "device initialized — %lux%lu",
                static_cast<unsigned long>(profile.lcd.width),
                static_cast<unsigned long>(profile.lcd.height));
    return BLUSYS_OK;
}

void device_deinit(blusys_lcd_t *lcd, blusys_display_t *ui)
{
    if (ui != nullptr) {
        blusys_display_close(ui);
    }
    if (lcd != nullptr) {
        blusys_lcd_close(lcd);
    }
    BLUSYS_LOGI(kTag, "device deinitialized");
}

void device_set_default_theme()
{
    blusys::set_theme(blusys::presets::expressive_dark());
}

void device_set_theme(const void *tokens)
{
    if (tokens != nullptr) {
        blusys::set_theme(*static_cast<const blusys::theme_tokens *>(tokens));
    }
}

std::uint32_t device_get_ticks_ms()
{
    return static_cast<std::uint32_t>(esp_timer_get_time() / 1000ULL);
}

void device_delay(std::uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void device_ui_lock(blusys_display_t *ui)
{
    blusys_display_lock(ui);
}

void device_ui_unlock(blusys_display_t *ui)
{
    blusys_display_unlock(ui);
}

void *device_find_pointer_indev()
{
    return find_first_pointer_indev();
}

}  // namespace blusys::platform

#endif  // BLUSYS_FRAMEWORK_HAS_UI
