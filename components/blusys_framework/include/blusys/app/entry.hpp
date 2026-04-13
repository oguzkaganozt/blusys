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
//   BLUSYS_APP(spec)                            — default interactive product when UI is
//                                                  enabled; headless when BLUSYS_FRAMEWORK_HAS_UI
//                                                  is off (e.g. headless_telemetry without LVGL)
//   BLUSYS_APP_INTERACTIVE(spec)                — handheld_starter profile (ST7735/ST7789)
//   BLUSYS_APP_DASHBOARD(spec, host_title)      — gateway/panel/dashboard SPI profile + SDL title
//   BLUSYS_APP_MAIN_HOST(spec)                  — interactive, SDL2 host
//   BLUSYS_APP_MAIN_HOST_PROFILE(spec, profile) — interactive host + explicit window size/title
//   BLUSYS_APP_MAIN_HEADLESS(spec)              — no UI, terminal
//   BLUSYS_APP_MAIN_HEADLESS_PROFILE(spec, hl)  — headless + headless_profile
//   BLUSYS_APP_MAIN_DEVICE(spec, profile)       — device target with LCD + optional encoder

#include "blusys/app/detail/app_runtime.hpp"
#include "blusys/app/host_profile.hpp"
#include "blusys/app/profiles/headless.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

// Device-specific headers — only available in the IDF component build
// (the host harness does not have blusys_services on its include path).
#if defined(BLUSYS_FRAMEWORK_HAS_UI) && defined(ESP_PLATFORM)
#include "blusys/app/platform_profile.hpp"
#include "blusys/app/input_bridge.hpp"
#include "blusys/app/button_array_bridge.hpp"
#include "blusys/app/touch_bridge.hpp"
#include "blusys/app/view/shell.hpp"
#include "blusys/output/display.h"
#endif

#if defined(BLUSYS_FRAMEWORK_HAS_UI) && !defined(ESP_PLATFORM)
#include "blusys/app/touch_bridge.hpp"
#endif

#if defined(BLUSYS_FRAMEWORK_HAS_UI)
#include "blusys/app/auto_profile.hpp"
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
void host_set_runtime(blusys::framework::runtime *rt);
// First LVGL POINTER indev (SDL mouse on host), or nullptr.
void *host_find_pointer_indev();
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
void *device_find_pointer_indev();
#endif  // ESP_PLATFORM
#endif  // BLUSYS_FRAMEWORK_HAS_UI

std::uint32_t headless_get_ticks_ms();
void headless_delay(std::uint32_t ms);

}  // namespace blusys_app_platform

namespace blusys::app::detail {

#ifdef BLUSYS_FRAMEWORK_HAS_UI

// identity->theme takes precedence over spec.theme (see app_spec.hpp).
template <typename State, typename Action>
const blusys::ui::theme_tokens *resolve_theme_tokens(const app_spec<State, Action> &spec)
{
    if (spec.identity != nullptr && spec.identity->theme != nullptr) {
        return spec.identity->theme;
    }
    return spec.theme;
}

// ---- host interactive entry (SDL2 + LVGL) ----

template <typename State, typename Action>
void run_host(const app_spec<State, Action> &spec,
              int hor_res = 480,
              int ver_res = 320,
              const char *title = "Blusys App")
{
    blusys_app_platform::host_init(hor_res, ver_res, title);

    const blusys::ui::theme_tokens *resolved_theme = resolve_theme_tokens(spec);
    if (resolved_theme != nullptr) {
        blusys_app_platform::host_set_theme(static_cast<const void *>(resolved_theme));
    } else {
        blusys_app_platform::host_set_default_theme();
    }

    app_runtime<State, Action> runtime(spec);
    blusys_err_t err = runtime.init();
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "app runtime init failed: %d", static_cast<int>(err));
        return;
    }

    // Bind framework runtime to host encoder so it posts intents
    // (increment, decrement, confirm, long_press, release) matching
    // the device input_bridge behavior.
    blusys_app_platform::host_set_runtime(&runtime.framework_runtime());

    // Pointer indev (SDL mouse): semantic tap / swipe intents for parity with device touch.
    blusys::app::touch_bridge host_touch{};
    {
        auto *ptr = static_cast<lv_indev_t *>(blusys_app_platform::host_find_pointer_indev());
        if (ptr != nullptr) {
            blusys::app::touch_bridge_config tcfg{};
            tcfg.framework_runtime = &runtime.framework_runtime();
            (void)blusys::app::touch_bridge_open(tcfg, ptr, &host_touch);
        }
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

    // LVGL 9: all API (theme, widget tree, indev) must run under the same lock
    // as lv_timer_handler() in the render task — otherwise concurrent access can
    // trip lv_inv_area assertions and wedge the main task.
    blusys_app_platform::device_ui_lock(ui);
    blusys_ui_invalidation_begin(ui);

    const blusys::ui::theme_tokens *resolved_theme = resolve_theme_tokens(spec);
    if (resolved_theme != nullptr) {
        blusys_app_platform::device_set_theme(static_cast<const void *>(resolved_theme));
    } else {
        blusys_app_platform::device_set_default_theme();
    }

    app_runtime<State, Action> runtime(spec);
    err = runtime.init();
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE("blusys_app", "app runtime init failed: %d", static_cast<int>(err));
        blusys_ui_invalidation_end(ui);
        blusys_app_platform::device_ui_unlock(ui);
        blusys_app_platform::device_deinit(lcd, ui);
        return;
    }

    // Input bridge (optional — only if profile declares an encoder).
    input_bridge bridge{};
    struct device_screen_changed_ctx {
        app_runtime<State, Action> *runtime = nullptr;
        bool                          has_shell = false;
    } screen_cb_ctx{};
    screen_cb_ctx.runtime   = &runtime;
    screen_cb_ctx.has_shell = (spec.shell != nullptr);

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
        } else {
            // Encoder indev is rebound by focus_scope_manager; refresh the
            // walking group after each transition. When a shell is present,
            // re-apply the same chrome updates as app_runtime::init (which
            // this callback overrides).
            runtime.screen_router().set_screen_changed_callback(
                [](lv_obj_t * /*screen*/, void *ctx) {
                    auto *p = static_cast<device_screen_changed_ctx *>(ctx);
                    if (p == nullptr || p->runtime == nullptr) {
                        return;
                    }
                    p->runtime->screen_router().focus_scopes().refresh_current();
                    if (p->has_shell) {
                        auto *sh = p->runtime->ctx().services().shell();
                        if (sh != nullptr) {
                            p->runtime->screen_router().sync_shell_chrome(*sh);
                            view::shell_set_back_visible(
                                *sh, p->runtime->screen_router().stack_depth() > 1);
                        }
                    }
                },
                &screen_cb_ctx);
        }
    }

    // Touch / pointer bridge (optional — uses first LVGL POINTER indev).
    blusys::app::touch_bridge touch{};
    if (profile.has_touch) {
        auto *ptr = static_cast<lv_indev_t *>(blusys_app_platform::device_find_pointer_indev());
        if (ptr != nullptr) {
            blusys::app::touch_bridge_config tcfg{};
            tcfg.framework_runtime = &runtime.framework_runtime();
            (void)blusys::app::touch_bridge_open(tcfg, ptr, &touch);
        } else {
            BLUSYS_LOGW("blusys_app",
                        "has_touch set but no LVGL POINTER indev — skipping touch bridge");
        }
    }

    blusys_ui_invalidation_end(ui);
    blusys_app_platform::device_ui_unlock(ui);

    // Button array bridge (optional — only if profile declares buttons).
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
        const std::uint32_t now = blusys_app_platform::device_get_ticks_ms();
        blusys_app_platform::device_ui_lock(ui);
        runtime.step(now);
        blusys_app_platform::device_ui_unlock(ui);
        blusys_app_platform::device_delay(spec.tick_period_ms);
    }

    // Unreachable in normal operation.
    if (btn_bridge.count > 0) {
        button_array_close(&btn_bridge);
    }
    if (profile.has_encoder) {
        input_bridge_close(&bridge);
    }
    blusys::app::touch_bridge_close(&touch);
    runtime.deinit();
    blusys_app_platform::device_deinit(lcd, ui);
}

#endif  // ESP_PLATFORM

#endif  // BLUSYS_FRAMEWORK_HAS_UI

// ---- headless entry (no UI, no LVGL) ----

template <typename State, typename Action>
void run_headless(const app_spec<State, Action> &spec,
                  const profiles::headless_profile &profile = profiles::headless_default())
{
    // Ensure log output is visible even when piped (stdout may be fully buffered).
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

// Like BLUSYS_APP_MAIN_HOST but reads window size and title from a
// blusys::app::host_profile struct. Useful for product profiles that
// have a fixed display size (e.g. 128×32 OLED, 320×240 TFT).
#define BLUSYS_APP_MAIN_HOST_PROFILE(spec_expr, profile_expr)               \
    int main(void) {                                                        \
        auto _blusys_spec    = (spec_expr);                                 \
        auto _blusys_profile = (profile_expr);                              \
        ::blusys::app::detail::run_host(_blusys_spec,                       \
                                        _blusys_profile.hor_res,            \
                                        _blusys_profile.ver_res,            \
                                        _blusys_profile.title);             \
        return 0;                                                           \
    }

#ifdef ESP_PLATFORM
#define BLUSYS_APP_MAIN_DEVICE(spec_expr, profile_expr)                     \
    extern "C" void app_main(void) {                                        \
        auto _blusys_spec = (spec_expr);                                    \
        auto _blusys_profile = (profile_expr);                              \
        ::blusys::app::detail::run_device(_blusys_spec, _blusys_profile);   \
    }

#define BLUSYS_APP_INTERACTIVE(spec_expr)                                   \
    extern "C" void app_main(void) {                                        \
        auto _blusys_spec = (spec_expr);                                    \
        ::blusys::app::detail::run_device(_blusys_spec,                     \
                                          ::blusys::app::auto_profile_interactive()); \
    }

#define BLUSYS_APP_DASHBOARD(spec_expr, host_title_literal)                 \
    extern "C" void app_main(void) {                                        \
        auto _blusys_spec = (spec_expr);                                    \
        (void)(host_title_literal);                                         \
        ::blusys::app::detail::run_device(_blusys_spec,                     \
                                          ::blusys::app::auto_profile_dashboard()); \
    }
#else
#define BLUSYS_APP_INTERACTIVE(spec_expr)                                   \
    int main(void) {                                                        \
        auto _blusys_spec = (spec_expr);                                    \
        const auto _hp = ::blusys::app::auto_host_profile_interactive();    \
        ::blusys::app::detail::run_host(_blusys_spec,                       \
                                        _hp.hor_res,                        \
                                        _hp.ver_res,                        \
                                        _hp.title);                         \
        return 0;                                                           \
    }

#define BLUSYS_APP_DASHBOARD(spec_expr, host_title_literal)                 \
    int main(void) {                                                        \
        auto _blusys_spec = (spec_expr);                                    \
        const auto _hp = ::blusys::app::auto_host_profile_dashboard(        \
            (host_title_literal));                                          \
        ::blusys::app::detail::run_host(_blusys_spec,                       \
                                        _hp.hor_res,                        \
                                        _hp.ver_res,                        \
                                        _hp.title);                         \
        return 0;                                                           \
    }
#endif  // ESP_PLATFORM

#define BLUSYS_APP(spec_expr) BLUSYS_APP_INTERACTIVE(spec_expr)

#endif  // BLUSYS_FRAMEWORK_HAS_UI

#if !defined(BLUSYS_FRAMEWORK_HAS_UI)
#define BLUSYS_APP(spec_expr) BLUSYS_APP_MAIN_HEADLESS(spec_expr)
#endif

#if defined(ESP_PLATFORM)
#define BLUSYS_APP_MAIN_HEADLESS(spec_expr)                                 \
    extern "C" void app_main(void) {                                        \
        auto _blusys_spec = (spec_expr);                                    \
        ::blusys::app::detail::run_headless(_blusys_spec);                  \
    }

// Like `BLUSYS_APP_MAIN_HEADLESS` but passes a `blusys::app::profiles::headless_profile`
// for code-first headless configuration (e.g. boot log label).
#define BLUSYS_APP_MAIN_HEADLESS_PROFILE(spec_expr, profile_expr)           \
    extern "C" void app_main(void) {                                        \
        auto _blusys_spec = (spec_expr);                                    \
        auto _blusys_hl     = (profile_expr);                                \
        ::blusys::app::detail::run_headless(_blusys_spec, _blusys_hl);      \
    }
#else
#define BLUSYS_APP_MAIN_HEADLESS(spec_expr)                                 \
    int main(void) {                                                        \
        auto _blusys_spec = (spec_expr);                                    \
        ::blusys::app::detail::run_headless(_blusys_spec);                  \
        return 0;                                                           \
    }

#define BLUSYS_APP_MAIN_HEADLESS_PROFILE(spec_expr, profile_expr)           \
    int main(void) {                                                        \
        auto _blusys_spec = (spec_expr);                                    \
        auto _blusys_hl     = (profile_expr);                                \
        ::blusys::app::detail::run_headless(_blusys_spec, _blusys_hl);      \
        return 0;                                                           \
    }
#endif
