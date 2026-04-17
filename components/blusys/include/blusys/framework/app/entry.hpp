#pragma once

// Entry macros for blusys::app products.
//
// These macros define the program entry point and run the app runtime loop.
// Product code uses exactly ONE macro at file scope, passing the app_spec.
//
// Available macros:
//   BLUSYS_APP(spec)          — interactive product (host SDL or device LCD+input).
//                               On ESP_PLATFORM: uses spec.profile or auto_profile_interactive().
//                               On host: uses spec.profile lcd size or 480x320, spec.host_title.
//                               When BLUSYS_FRAMEWORK_HAS_UI is off: behaves as BLUSYS_APP_HEADLESS.
//   BLUSYS_APP_HEADLESS(spec) — no UI, no LVGL; runs the headless runtime loop.
//
// Platform and display selection live in spec, not in macro names:
//   .profile    = &blusys::platform::st7735_160x128()  ← pin display profile
//   .host_title = "My Dashboard"                        ← SDL window title
//   .flows      = {&kSettingsFlow}                      ← spec-selectable flows

#include "blusys/framework/app/internal/app_runtime.hpp"
#include "blusys/framework/platform/host.hpp"
#include "blusys/framework/platform/headless.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

// Device-specific headers — only available in the IDF component build.
#if defined(BLUSYS_FRAMEWORK_HAS_UI) && defined(ESP_PLATFORM)
#include "blusys/framework/platform/profile.hpp"
#include "blusys/framework/platform/input_bridge.hpp"
#include "blusys/framework/platform/button_array_bridge.hpp"
#include "blusys/framework/platform/touch_bridge.hpp"
#include "blusys/framework/ui/composition/shell.hpp"
#include "blusys/drivers/display.h"
#endif

#if defined(BLUSYS_FRAMEWORK_HAS_UI) && !defined(ESP_PLATFORM)
#include "blusys/framework/platform/profile.hpp"
#include "blusys/framework/platform/touch_bridge.hpp"
#endif

#if defined(BLUSYS_FRAMEWORK_HAS_UI)
#include "blusys/framework/platform/auto.hpp"
#include "blusys/framework/ui/style/presets.hpp"
#endif

#include <cstdint>
#include <cstdio>

// ---- platform function declarations (resolved at link time) ----

namespace blusys::platform {

#ifdef BLUSYS_FRAMEWORK_HAS_UI
void host_init(int w, int h, const char *title);
void host_set_runtime(blusys::runtime *rt);
void *host_find_pointer_indev();
std::uint32_t host_get_ticks_ms();
void host_tick_inc(std::uint32_t ms);
std::uint32_t host_frame_step();
void host_frame_delay(std::uint32_t ms);
void host_set_theme(const void *tokens);

#ifdef ESP_PLATFORM
blusys_err_t device_init(blusys::device_profile &profile,
                         blusys_lcd_t **lcd_out,
                         blusys_display_t **ui_out);
void device_deinit(blusys_lcd_t *lcd, blusys_display_t *ui);
void device_set_theme(const void *tokens);
std::uint32_t device_get_ticks_ms();
void device_delay(std::uint32_t ms);
void device_ui_lock(blusys_display_t *ui);
void device_ui_unlock(blusys_display_t *ui);
void *device_find_pointer_indev();
#endif  // ESP_PLATFORM
#endif  // BLUSYS_FRAMEWORK_HAS_UI

std::uint32_t headless_get_ticks_ms();
void headless_delay(std::uint32_t ms);

}  // namespace blusys::platform

namespace blusys::detail {

#ifdef BLUSYS_FRAMEWORK_HAS_UI

// identity->theme takes precedence over spec.theme.
template <typename State, typename Action>
const blusys::theme_tokens *resolve_theme_tokens(const app_spec<State, Action> &spec)
{
    if (spec.identity != nullptr && spec.identity->theme != nullptr) {
        return spec.identity->theme;
    }
    return spec.theme;
}

// ---- host interactive entry (SDL2 + LVGL) ----

template <typename State, typename Action>
void run_host(const app_spec<State, Action> &spec)
{
    // Window size: from spec.profile if set, else default 480x320.
    int hor_res = 480;
    int ver_res = 320;
    if (spec.profile != nullptr && spec.profile->lcd.width > 0) {
        hor_res = static_cast<int>(spec.profile->lcd.width);
        ver_res = static_cast<int>(spec.profile->lcd.height);
    }
    const char *title = (spec.host_title != nullptr) ? spec.host_title : "Blusys App";

    platform::host_init(hor_res, ver_res, title);

    const blusys::theme_tokens *resolved_theme = resolve_theme_tokens(spec);
    const blusys::theme_tokens scaled_theme =
        (resolved_theme != nullptr)
            ? blusys::presets::for_display(*resolved_theme,
                  static_cast<std::uint32_t>(hor_res),
                  static_cast<std::uint32_t>(ver_res),
                  blusys::presets::panel_kind::rgb565)
            : blusys::presets::for_display(
                  static_cast<std::uint32_t>(hor_res),
                  static_cast<std::uint32_t>(ver_res),
                  blusys::presets::panel_kind::rgb565);
    platform::host_set_theme(static_cast<const void *>(&scaled_theme));

    app_runtime<State, Action> runtime(spec);
    blusys_err_t err = runtime.init();
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "app runtime init failed: %d", static_cast<int>(err));
        return;
    }

    platform::host_set_runtime(&runtime.framework_runtime());

    blusys::touch_bridge host_touch{};
    {
        auto *ptr = static_cast<lv_indev_t *>(platform::host_find_pointer_indev());
        if (ptr != nullptr) {
            blusys::touch_bridge_config tcfg{};
            tcfg.framework_runtime = &runtime.framework_runtime();
            (void)blusys::touch_bridge_open(tcfg, ptr, &host_touch);
        }
    }

    BLUSYS_LOGI("blusys_app", "app running");

    std::uint32_t last_ticks = platform::host_get_ticks_ms();
    while (true) {
        const std::uint32_t now = platform::host_get_ticks_ms();
        const std::uint32_t elapsed = now - last_ticks;
        if (elapsed > 0) {
            platform::host_tick_inc(elapsed);
            last_ticks = now;
        }
        runtime.step(now);
        const std::uint32_t sleep_ms = platform::host_frame_step();
        platform::host_frame_delay(sleep_ms < 5 ? 5 : (sleep_ms > 50 ? 50 : sleep_ms));
    }

    runtime.deinit();
}

// ---- device entry (ESP-IDF + LCD + LVGL) ----

#ifdef ESP_PLATFORM

template <typename State, typename Action>
void run_device(const app_spec<State, Action> &spec)
{
    // Profile: use spec.profile if set, otherwise fall back to auto_profile_interactive().
    device_profile profile = (spec.profile != nullptr)
                             ? *spec.profile
                             : blusys::auto_profile_interactive();

    blusys_lcd_t *lcd = nullptr;
    blusys_display_t  *ui  = nullptr;

    blusys_err_t err = platform::device_init(profile, &lcd, &ui);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "device init failed: %d", static_cast<int>(err));
        return;
    }

    platform::device_ui_lock(ui);
    blusys_display_invalidation_begin(ui);

    const blusys::theme_tokens *resolved_theme = resolve_theme_tokens(spec);
    const blusys::theme_tokens scaled_theme =
        (resolved_theme != nullptr)
            ? blusys::presets::for_display(*resolved_theme,
                  static_cast<std::uint32_t>(profile.lcd.width),
                  static_cast<std::uint32_t>(profile.lcd.height),
                  static_cast<blusys::presets::panel_kind>(profile.ui.panel_kind))
            : blusys::presets::for_display(
                  static_cast<std::uint32_t>(profile.lcd.width),
                  static_cast<std::uint32_t>(profile.lcd.height),
                  static_cast<blusys::presets::panel_kind>(profile.ui.panel_kind));
    platform::device_set_theme(static_cast<const void *>(&scaled_theme));

    app_runtime<State, Action> runtime(spec);
    err = runtime.init();
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "app runtime init failed: %d", static_cast<int>(err));
        blusys_display_invalidation_end(ui);
        platform::device_ui_unlock(ui);
        platform::device_deinit(lcd, ui);
        return;
    }

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

    blusys::touch_bridge touch{};
    if (profile.has_touch) {
        auto *ptr = static_cast<lv_indev_t *>(platform::device_find_pointer_indev());
        if (ptr != nullptr) {
            blusys::touch_bridge_config tcfg{};
            tcfg.framework_runtime = &runtime.framework_runtime();
            (void)blusys::touch_bridge_open(tcfg, ptr, &touch);
        } else {
            BLUSYS_LOGW("blusys_app",
                        "has_touch set but no LVGL POINTER indev — skipping touch bridge");
        }
    }

    blusys_display_invalidation_end(ui);
    platform::device_ui_unlock(ui);

    button_array_bridge btn_bridge{};
    if (profile.button_count > 0 && profile.buttons != nullptr) {
        button_array_config btn_cfg{};
        btn_cfg.count = profile.button_count;
        btn_cfg.framework_runtime = &runtime.framework_runtime();
        for (std::size_t i = 0; i < profile.button_count && i < kMaxButtons; ++i) {
            btn_cfg.buttons[i] = {
                .button_config = profile.buttons[i].button_config,
                .on_press      = profile.buttons[i].on_press,
                .on_long_press = profile.buttons[i].on_long_press,
            };
        }
        const blusys_err_t btn_err = button_array_open(btn_cfg, &btn_bridge);
        if (btn_err != BLUSYS_OK) {
            BLUSYS_LOGW("blusys_app",
                        "button array open failed: %d — continuing without buttons",
                        static_cast<int>(btn_err));
        }
    }

    BLUSYS_LOGI("blusys_app", "app running on device");

    while (true) {
        const std::uint32_t now = platform::device_get_ticks_ms();
        platform::device_ui_lock(ui);
        runtime.step(now);
        platform::device_ui_unlock(ui);
        platform::device_delay(spec.tick_period_ms);
    }

    // Unreachable in normal operation.
    if (btn_bridge.count > 0) {
        button_array_close(&btn_bridge);
    }
    if (profile.has_encoder) {
        input_bridge_close(&bridge);
    }
    blusys::touch_bridge_close(&touch);
    runtime.deinit();
    platform::device_deinit(lcd, ui);
}

#endif  // ESP_PLATFORM

#endif  // BLUSYS_FRAMEWORK_HAS_UI

// ---- headless entry (no UI, no LVGL) ----

template <typename State, typename Action>
void run_headless(const app_spec<State, Action> &spec,
                  const platform::headless_profile &profile = platform::headless_default())
{
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    app_runtime<State, Action> runtime(spec);
    blusys_err_t err = runtime.init();
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "app runtime init failed: %d", static_cast<int>(err));
        return;
    }

    if (profile.boot_log_label != nullptr && profile.boot_log_label[0] != '\0') {
        BLUSYS_LOGI("blusys_app", "headless app running (%s)", profile.boot_log_label);
    } else {
        BLUSYS_LOGI("blusys_app", "headless app running");
    }

    while (true) {
        const std::uint32_t now = platform::headless_get_ticks_ms();
        runtime.step(now);
        platform::headless_delay(spec.tick_period_ms);
    }

    runtime.deinit();
}

}  // namespace blusys::detail

// ---- entry macros ----

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#ifdef ESP_PLATFORM

#define BLUSYS_APP(spec_expr)                                           \
    extern "C" void app_main(void) {                                    \
        auto _blusys_spec = (spec_expr);                                \
        ::blusys::detail::run_device(_blusys_spec);                     \
    }

#else  // host

#define BLUSYS_APP(spec_expr)                                           \
    int main(void) {                                                    \
        auto _blusys_spec = (spec_expr);                                \
        ::blusys::detail::run_host(_blusys_spec);                       \
        return 0;                                                       \
    }

#endif  // ESP_PLATFORM

#else  // !BLUSYS_FRAMEWORK_HAS_UI — BLUSYS_APP falls through to headless

#define BLUSYS_APP(spec_expr) BLUSYS_APP_HEADLESS(spec_expr)

#endif  // BLUSYS_FRAMEWORK_HAS_UI

// BLUSYS_APP_HEADLESS — no UI, no LVGL. Link-time decision: headless apps
// genuinely don't link LVGL, so this cannot collapse into BLUSYS_APP.

#if defined(ESP_PLATFORM)

#define BLUSYS_APP_HEADLESS(spec_expr)                                  \
    extern "C" void app_main(void) {                                    \
        auto _blusys_spec = (spec_expr);                                \
        ::blusys::detail::run_headless(_blusys_spec);                   \
    }

#else

#define BLUSYS_APP_HEADLESS(spec_expr)                                  \
    int main(void) {                                                    \
        auto _blusys_spec = (spec_expr);                                \
        ::blusys::detail::run_headless(_blusys_spec);                   \
        return 0;                                                       \
    }

#endif
