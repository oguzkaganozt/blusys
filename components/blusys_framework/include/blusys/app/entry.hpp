#pragma once

// Entry macros for blusys::app products.
//
// These macros define the program entry point and run the app runtime
// loop. Product code should use exactly one of these macros at file
// scope, passing in the app_spec to run.
//
// The macros hide:
//   - main() / app_main() definition
//   - LVGL init and display setup (interactive path)
//   - LCD and UI service bring-up (device path)
//   - encoder input wiring (device path)
//   - theme selection and application
//   - runtime construction and main loop
//
// Available entry macros:
//   BLUSYS_APP_MAIN_HOST(spec)            — interactive, SDL2 host
//   BLUSYS_APP_MAIN_HEADLESS(spec)        — no UI, terminal
//   BLUSYS_APP_MAIN_DEVICE(spec, profile) — device target with LCD + optional encoder

#include "blusys/app/app_runtime.hpp"
#include "blusys/log.h"

// Device-specific headers — only available in the IDF component build
// (the host harness does not have blusys_services on its include path).
#if defined(BLUSYS_FRAMEWORK_HAS_UI) && defined(ESP_PLATFORM)
#include "blusys/app/platform_profile.hpp"
#include "blusys/app/input_bridge.hpp"
#endif

#include <cstdint>
#include <cstdio>

// ---- platform function declarations (resolved at link time) ----
//
// These are defined in platform-specific .cpp files (e.g. app_host_platform.cpp).
// They use C++ linkage in a flat namespace to avoid mangling issues.

namespace blusys_app_platform {

#ifdef BLUSYS_FRAMEWORK_HAS_UI
// Host platform (SDL2)
void host_init(int w, int h, const char *title);
std::uint32_t host_get_ticks_ms();
void host_tick_inc(std::uint32_t ms);
std::uint32_t host_frame_step();
void host_frame_delay(std::uint32_t ms);
void host_set_default_theme();
void host_set_theme(const void *tokens);

#ifdef ESP_PLATFORM
// Device platform (ESP-IDF)
blusys_err_t device_init(blusys::app::device_profile &profile,
                         blusys_lcd_t **lcd_out,
                         blusys_ui_t **ui_out);
void device_deinit(blusys_lcd_t *lcd, blusys_ui_t *ui);
void device_set_default_theme();
void device_set_theme(const void *tokens);
std::uint32_t device_get_ticks_ms();
void device_delay(std::uint32_t ms);
void device_ui_lock(blusys_ui_t *ui);
void device_ui_unlock(blusys_ui_t *ui);
#endif  // ESP_PLATFORM
#endif  // BLUSYS_FRAMEWORK_HAS_UI

std::uint32_t headless_get_ticks_ms();
void headless_delay(std::uint32_t ms);

}  // namespace blusys_app_platform

namespace blusys::app::detail {

// ---- host interactive entry (SDL2 + LVGL) ----

#ifdef BLUSYS_FRAMEWORK_HAS_UI

template <typename State, typename Action>
void run_host(const app_spec<State, Action> &spec,
              int hor_res = 480,
              int ver_res = 320,
              const char *title = "Blusys App")
{
    blusys_app_platform::host_init(hor_res, ver_res, title);

    if (spec.theme != nullptr) {
        blusys_app_platform::host_set_theme(static_cast<const void *>(spec.theme));
    } else {
        blusys_app_platform::host_set_default_theme();
    }

    app_runtime<State, Action> runtime(spec);
    blusys_err_t err = runtime.init();
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "app runtime init failed: %d", static_cast<int>(err));
        return;
    }

    BLUSYS_LOGI("blusys_app", "app running");

    std::uint32_t last_ticks = blusys_app_platform::host_get_ticks_ms();
    while (true) {
        const std::uint32_t now = blusys_app_platform::host_get_ticks_ms();
        const std::uint32_t elapsed = now - last_ticks;
        if (elapsed > 0) {
            blusys_app_platform::host_tick_inc(elapsed);
            last_ticks = now;
        }
        runtime.step(now);
        const std::uint32_t sleep_ms = blusys_app_platform::host_frame_step();
        blusys_app_platform::host_frame_delay(sleep_ms < 5 ? 5 : (sleep_ms > 50 ? 50 : sleep_ms));
    }

    runtime.deinit();
}

// ---- device entry (ESP-IDF + LCD + LVGL) ----

#ifdef ESP_PLATFORM

template <typename State, typename Action>
void run_device(const app_spec<State, Action> &spec,
                device_profile profile)
{
    blusys_lcd_t *lcd = nullptr;
    blusys_ui_t  *ui  = nullptr;

    blusys_err_t err = blusys_app_platform::device_init(profile, &lcd, &ui);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "device init failed: %d", static_cast<int>(err));
        return;
    }

    if (spec.theme != nullptr) {
        blusys_app_platform::device_set_theme(static_cast<const void *>(spec.theme));
    } else {
        blusys_app_platform::device_set_default_theme();
    }

    app_runtime<State, Action> runtime(spec);
    err = runtime.init();
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "app runtime init failed: %d", static_cast<int>(err));
        blusys_app_platform::device_deinit(lcd, ui);
        return;
    }

    // Input bridge (optional — only if profile declares an encoder).
    input_bridge bridge{};
    if (profile.has_encoder) {
        input_bridge_config bridge_cfg{};
        bridge_cfg.encoder_config = profile.encoder;
        bridge_cfg.framework_runtime = &runtime.framework_runtime();
        const blusys_err_t bridge_err = input_bridge_open(bridge_cfg, &bridge);
        if (bridge_err != BLUSYS_OK) {
            BLUSYS_LOGW("blusys_app",
                        "input bridge open failed: %d — continuing without encoder",
                        static_cast<int>(bridge_err));
            profile.has_encoder = false;
        }
    }

    BLUSYS_LOGI("blusys_app", "app running on device");

    while (true) {
        const std::uint32_t now = blusys_app_platform::device_get_ticks_ms();
        blusys_app_platform::device_ui_lock(ui);
        runtime.step(now);
        blusys_app_platform::device_ui_unlock(ui);
        blusys_app_platform::device_delay(spec.tick_period_ms);
    }

    // Unreachable in normal operation.
    if (profile.has_encoder) {
        input_bridge_close(&bridge);
    }
    runtime.deinit();
    blusys_app_platform::device_deinit(lcd, ui);
}

#endif  // ESP_PLATFORM

#endif  // BLUSYS_FRAMEWORK_HAS_UI

// ---- headless entry (no UI, no LVGL) ----

template <typename State, typename Action>
void run_headless(const app_spec<State, Action> &spec)
{
    // Ensure log output is visible even when piped (stdout may be fully buffered).
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    app_runtime<State, Action> runtime(spec);
    blusys_err_t err = runtime.init();
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "app runtime init failed: %d", static_cast<int>(err));
        return;
    }

    BLUSYS_LOGI("blusys_app", "headless app running");

    while (true) {
        const std::uint32_t now = blusys_app_platform::headless_get_ticks_ms();
        runtime.step(now);
        blusys_app_platform::headless_delay(spec.tick_period_ms);
    }

    runtime.deinit();
}

}  // namespace blusys::app::detail

// ---- entry macros ----

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#define BLUSYS_APP_MAIN_HOST(spec_expr)                                     \
    int main(void) {                                                        \
        auto _blusys_spec = (spec_expr);                                    \
        ::blusys::app::detail::run_host(_blusys_spec);                      \
        return 0;                                                           \
    }

#ifdef ESP_PLATFORM
#define BLUSYS_APP_MAIN_DEVICE(spec_expr, profile_expr)                     \
    extern "C" void app_main(void) {                                        \
        auto _blusys_spec = (spec_expr);                                    \
        auto _blusys_profile = (profile_expr);                              \
        ::blusys::app::detail::run_device(_blusys_spec, _blusys_profile);   \
    }
#endif  // ESP_PLATFORM

#endif  // BLUSYS_FRAMEWORK_HAS_UI

#define BLUSYS_APP_MAIN_HEADLESS(spec_expr)                                 \
    int main(void) {                                                        \
        auto _blusys_spec = (spec_expr);                                    \
        ::blusys::app::detail::run_headless(_blusys_spec);                  \
        return 0;                                                           \
    }
