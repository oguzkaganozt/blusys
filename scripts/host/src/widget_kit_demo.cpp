// scripts/host/src/widget_kit_demo.cpp — Phase 9 framework bridge demo.
//
// Opens an SDL2 window via LVGL's bundled SDL display driver, just like
// `hello_lvgl`, but builds the screen through the blusys framework's
// widget kit instead of raw LVGL calls. The point of this executable is
// threefold:
//
//   1. Prove the framework C++ surface (core spine + widget kit +
//      layout primitives) compiles cleanly against a host LVGL.
//   2. Give the widget kit a visual iteration loop that does not
//      require flashing hardware — product designers can tweak the
//      theme tokens here and see the result in seconds.
//   3. Exercise the full spine (runtime → controller → route sink)
//      end-to-end on host so Phase 9 stage-1 (PC + LVGL + SDL2) has
//      a concrete artifact. The screen has three buttons that post
//      `intent::decrement`, `intent::increment`, and `intent::confirm`
//      to a runtime, and a small controller mutates the slider /
//      submits a `show_overlay` route command in response — i.e. the
//      same chain framework_app_basic validates on device.
//
// The mouse acts as the "encoder" on host: clicking a button drives the
// runtime exactly the way a rotary encoder + press would on hardware.
// (The encoder_basic example wires a real lv_indev_t of type
// LV_INDEV_TYPE_ENCODER; adding keyboard-driven encoder simulation to
// this demo is a later task.)

#include <SDL2/SDL.h>
#include <cstdint>
#include <cstdio>

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_keyboard.h"

#include "blusys/framework/framework.hpp"
#include "blusys/framework/ui/widgets.hpp"
#include "blusys/log.h"

namespace {

constexpr const char *kTag = "widget_kit_demo";

constexpr int32_t kSliderMin  = 0;
constexpr int32_t kSliderMax  = 100;
constexpr int32_t kSliderStep = 5;
constexpr int32_t kSliderInit = 50;

constexpr int kHorRes = 480;
constexpr int kVerRes = 320;

int32_t clamp_slider(int32_t value)
{
    if (value < kSliderMin) return kSliderMin;
    if (value > kSliderMax) return kSliderMax;
    return value;
}

// ---- shared app state -------------------------------------------------------

struct app_state {
    lv_obj_t *slider = nullptr;
    lv_obj_t *toast  = nullptr;
};

// ---- feedback sink: host-side logger ---------------------------------------

class logging_feedback_sink final : public blusys::framework::feedback_sink {
public:
    bool supports(blusys::framework::feedback_channel) const override
    {
        return true;
    }

    void emit(const blusys::framework::feedback_event &event) override
    {
        BLUSYS_LOGI(kTag,
                    "feedback: channel=%s pattern=%s value=%lu",
                    blusys::framework::feedback_channel_name(event.channel),
                    blusys::framework::feedback_pattern_name(event.pattern),
                    static_cast<unsigned long>(event.value));
    }
};

// ---- route sink: applies route commands to real widgets --------------------

class ui_route_sink final : public blusys::framework::route_sink {
public:
    void bind(app_state *state) { state_ = state; }

    void submit(const blusys::framework::route_command &command) override
    {
        BLUSYS_LOGI(kTag,
                    "route: %s id=%lu",
                    blusys::framework::route_command_type_name(command.type),
                    static_cast<unsigned long>(command.id));

        if (state_ == nullptr) return;

        switch (command.type) {
        case blusys::framework::route_command_type::show_overlay:
            blusys::ui::overlay_show(state_->toast);
            break;
        case blusys::framework::route_command_type::hide_overlay:
            blusys::ui::overlay_hide(state_->toast);
            break;
        default:
            break;
        }
    }

private:
    app_state *state_ = nullptr;
};

// ---- controller -------------------------------------------------------------

class demo_controller final : public blusys::framework::controller {
public:
    void bind(app_state *state) { state_ = state; }

    blusys_err_t init() override
    {
        emit_feedback({
            .channel = blusys::framework::feedback_channel::visual,
            .pattern = blusys::framework::feedback_pattern::focus,
            .value   = 1,
            .payload = nullptr,
        });
        return BLUSYS_OK;
    }

    void handle(const blusys::framework::app_event &event) override
    {
        if (event.kind != blusys::framework::app_event_kind::intent) return;
        if (state_ == nullptr || state_->slider == nullptr) return;

        switch (blusys::framework::app_event_intent(event)) {
        case blusys::framework::intent::increment: {
            const int32_t cur = blusys::ui::slider_get_value(state_->slider);
            blusys::ui::slider_set_value(state_->slider, clamp_slider(cur + kSliderStep));
            emit_click_feedback();
            break;
        }
        case blusys::framework::intent::decrement: {
            const int32_t cur = blusys::ui::slider_get_value(state_->slider);
            blusys::ui::slider_set_value(state_->slider, clamp_slider(cur - kSliderStep));
            emit_click_feedback();
            break;
        }
        case blusys::framework::intent::confirm: {
            submit_route(blusys::framework::route::show_overlay(1));
            emit_feedback({
                .channel = blusys::framework::feedback_channel::audio,
                .pattern = blusys::framework::feedback_pattern::confirm,
                .value   = 1,
                .payload = nullptr,
            });
            break;
        }
        default:
            break;
        }
    }

private:
    void emit_click_feedback() const
    {
        emit_feedback({
            .channel = blusys::framework::feedback_channel::haptic,
            .pattern = blusys::framework::feedback_pattern::click,
            .value   = 1,
            .payload = nullptr,
        });
    }

    app_state *state_ = nullptr;
};

// ---- module-level singletons ------------------------------------------------

app_state                   g_state{};
demo_controller             g_controller{};
ui_route_sink               g_route_sink{};
logging_feedback_sink       g_feedback_sink{};
blusys::framework::runtime  g_runtime{};
blusys::framework::runtime *g_runtime_ptr = &g_runtime;

void build_demo_screen()
{
    // Theme tokens — matches framework_app_basic so the host visual
    // matches the device visual. Tweak these freely when iterating.
    blusys::ui::set_theme({
        .color_primary    = lv_color_hex(0x2A62FF),
        .color_surface    = lv_color_hex(0x11141D),
        .color_on_primary = lv_color_hex(0xF7F9FF),
        .color_accent     = lv_color_hex(0x69D6C8),
        .color_disabled   = lv_color_hex(0x667089),
        .spacing_sm   = 8,
        .spacing_md   = 12,
        .spacing_lg   = 20,
        .radius_card  = 14,
        .radius_button = 999,
        .font_body   = LV_FONT_DEFAULT,
        .font_title  = LV_FONT_DEFAULT,
        .font_mono   = LV_FONT_DEFAULT,
    });

    lv_obj_t *screen = blusys::ui::screen_create({});
    lv_obj_t *col = blusys::ui::col_create(screen, {
        .gap     = blusys::ui::theme().spacing_md,
        .padding = blusys::ui::theme().spacing_lg,
    });

    blusys::ui::label_create(col, {
        .text = "Blusys widget kit",
        .font = blusys::ui::theme().font_title,
    });
    blusys::ui::divider_create(col, {});
    blusys::ui::label_create(col, {
        .text = "Volume",
        .font = blusys::ui::theme().font_body,
    });

    g_state.slider = blusys::ui::slider_create(col, {
        .min       = kSliderMin,
        .max       = kSliderMax,
        .initial   = kSliderInit,
        .on_change = nullptr,
        .user_data = nullptr,
        .disabled  = false,
    });

    lv_obj_t *button_row = blusys::ui::row_create(col, {
        .gap     = blusys::ui::theme().spacing_sm,
        .padding = 0,
    });

    blusys::ui::button_create(button_row, {
        .label   = "Down",
        .variant = blusys::ui::button_variant::secondary,
        .on_press = +[](void *user_data) {
            auto *runtime = static_cast<blusys::framework::runtime *>(user_data);
            runtime->post_intent(blusys::framework::intent::decrement);
        },
        .user_data = g_runtime_ptr,
    });
    blusys::ui::button_create(button_row, {
        .label   = "Up",
        .variant = blusys::ui::button_variant::secondary,
        .on_press = +[](void *user_data) {
            auto *runtime = static_cast<blusys::framework::runtime *>(user_data);
            runtime->post_intent(blusys::framework::intent::increment);
        },
        .user_data = g_runtime_ptr,
    });
    blusys::ui::button_create(button_row, {
        .label   = "OK",
        .variant = blusys::ui::button_variant::primary,
        .on_press = +[](void *user_data) {
            auto *runtime = static_cast<blusys::framework::runtime *>(user_data);
            runtime->post_intent(blusys::framework::intent::confirm);
        },
        .user_data = g_runtime_ptr,
    });

    g_state.toast = blusys::ui::overlay_create(screen, {
        .text        = "Confirmed",
        .duration_ms = 1500,
        .on_hidden   = nullptr,
        .user_data   = nullptr,
    });
}

}  // namespace

int main(void)
{
    lv_init();

    lv_display_t *display = lv_sdl_window_create(kHorRes, kVerRes);
    if (display == nullptr) {
        std::fprintf(stderr, "lv_sdl_window_create failed\n");
        return 1;
    }
    lv_sdl_window_set_title(display, "Blusys widget kit — host harness");

    (void)lv_sdl_mouse_create();
    (void)lv_sdl_keyboard_create();

    blusys::framework::init();
    build_demo_screen();

    // Spine wiring — same order as framework_app_basic.
    g_controller.bind(&g_state);
    g_route_sink.bind(&g_state);
    g_runtime.register_feedback_sink(&g_feedback_sink);

    const blusys_err_t init_err = g_runtime.init(&g_controller, &g_route_sink, 10);
    if (init_err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "runtime.init failed: %d", static_cast<int>(init_err));
        return 1;
    }

    BLUSYS_LOGI(kTag, "widget kit demo running. close the window to exit.");
    BLUSYS_LOGI(kTag, "initial slider = %ld",
                static_cast<long>(blusys::ui::slider_get_value(g_state.slider)));

    // Main loop: drive LVGL ticks + timers and step the runtime each
    // frame so queued intents (from button clicks) are processed.
    std::uint32_t last_ticks = SDL_GetTicks();
    while (true) {
        const std::uint32_t now = SDL_GetTicks();
        const std::uint32_t elapsed = now - last_ticks;
        if (elapsed > 0) {
            lv_tick_inc(elapsed);
            last_ticks = now;
        }
        g_runtime.step(now);
        const std::uint32_t sleep_ms = lv_timer_handler();
        SDL_Delay(sleep_ms < 5 ? 5 : (sleep_ms > 50 ? 50 : sleep_ms));
    }

    return 0;
}
