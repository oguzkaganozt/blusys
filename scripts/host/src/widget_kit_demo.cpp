// scripts/host/src/widget_kit_demo.cpp — framework bridge demo.
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
//   3. Exercise the full spine (runtime → runtime_handler → route sink)
//      end-to-end on host (PC + LVGL + SDL2) so CI and local dev have
//      a concrete artifact. The screen has three buttons that post
//      `intent::decrement`, `intent::increment`, and `intent::confirm`
//      to a runtime, and a small event handler mutates the slider /
//      submits a `show_overlay` route command in response — i.e. the
//      same chain framework_ui_basic (spine scenario) validates on device.
//
// Input works two ways on host:
//
//   Mouse — clicking a button drives the runtime exactly the way a
//   rotary encoder + press would on hardware.
//
//   Keyboard encoder simulation — arrow keys (Left/Right or Up/Down)
//   map to LV_INDEV_TYPE_ENCODER rotation, Space/Enter maps to the
//   encoder button press. This drives LVGL focus traversal and widget
//   activation through the same create_encoder_group + auto_focus_screen
//   helpers used on real hardware (framework_ui_basic encoder scenario).

#include <SDL2/SDL.h>
#include <cstdint>
#include <cstdio>

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_keyboard.h"

#include "blusys/framework/framework.hpp"
#include "blusys/framework/ui/input/encoder.hpp"
#include "blusys/framework/ui/widgets.hpp"
#include "blusys/framework/ui/style/presets.hpp"
#include "blusys/hal/log.h"

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

// ---- keyboard → encoder bridge ----------------------------------------------
//
// Maps SDL keyboard events to LVGL encoder state. Same pattern as
// framework_ui_basic encoder scenario's hardware encoder bridge, but fed by
// keyboard instead of quadrature hardware.

struct encoder_state {
    int32_t          pending_diff = 0;
    lv_indev_state_t button_state = LV_INDEV_STATE_RELEASED;
};

encoder_state g_encoder_state{};

int sdl_encoder_key_watch(void * /*userdata*/, SDL_Event *event)
{
    if (event->type != SDL_KEYDOWN && event->type != SDL_KEYUP) return 0;

    const SDL_Keycode key = event->key.keysym.sym;

    if (event->type == SDL_KEYDOWN) {
        switch (key) {
        case SDLK_LEFT:  // fallthrough
        case SDLK_DOWN:
            g_encoder_state.pending_diff -= 1;
            break;
        case SDLK_RIGHT: // fallthrough
        case SDLK_UP:
            g_encoder_state.pending_diff += 1;
            break;
        case SDLK_SPACE:  // fallthrough
        case SDLK_RETURN:
            if (event->key.repeat == 0)
                g_encoder_state.button_state = LV_INDEV_STATE_PRESSED;
            break;
        default:
            break;
        }
    } else if (event->type == SDL_KEYUP) {
        switch (key) {
        case SDLK_SPACE:  // fallthrough
        case SDLK_RETURN:
            g_encoder_state.button_state = LV_INDEV_STATE_RELEASED;
            break;
        default:
            break;
        }
    }
    return 0;  // non-consuming — let LVGL's own SDL driver see events too
}

void encoder_indev_read_cb(lv_indev_t * /*indev*/, lv_indev_data_t *data)
{
    data->enc_diff               = g_encoder_state.pending_diff;
    data->state                  = g_encoder_state.button_state;
    g_encoder_state.pending_diff = 0;
}

// ---- shared app state -------------------------------------------------------

struct app_state {
    lv_obj_t *screen = nullptr;
    lv_obj_t *slider = nullptr;
    lv_obj_t *toast  = nullptr;
};

// ---- feedback sink: host-side logger ---------------------------------------

class logging_feedback_sink final : public blusys::feedback_sink {
public:
    bool supports(blusys::feedback_channel) const override
    {
        return true;
    }

    void emit(const blusys::feedback_event &event) override
    {
        BLUSYS_LOGI(kTag,
                    "feedback: channel=%s pattern=%s value=%lu",
                    blusys::feedback_channel_name(event.channel),
                    blusys::feedback_pattern_name(event.pattern),
                    static_cast<unsigned long>(event.value));
    }
};

// ---- route sink: applies route commands to real widgets --------------------

class ui_route_sink final : public blusys::route_sink {
public:
    void bind(app_state *state) { state_ = state; }

    void submit(const blusys::route_command &command) override
    {
        BLUSYS_LOGI(kTag,
                    "route: %s id=%lu",
                    blusys::route_command_type_name(command.type),
                    static_cast<unsigned long>(command.id));

        if (state_ == nullptr) return;

        switch (command.type) {
        case blusys::route_command_type::show_overlay:
            blusys::overlay_show(state_->toast);
            break;
        case blusys::route_command_type::hide_overlay:
            blusys::overlay_hide(state_->toast);
            break;
        default:
            break;
        }
    }

private:
    app_state *state_ = nullptr;
};

// ---- runtime_handler callbacks (replaces legacy controller subclass) --------

struct demo_handler_ctx {
    app_state *state = nullptr;
};

demo_handler_ctx g_demo_ctx{};

static blusys_err_t demo_on_init(void *ctx, blusys::feedback_bus *fb)
{
    (void)ctx;
    blusys::feedback_dispatch(fb, {
        .channel = blusys::feedback_channel::visual,
        .pattern = blusys::feedback_pattern::focus,
        .value   = 1,
        .payload = nullptr,
    });
    return BLUSYS_OK;
}

static void demo_on_event(void *ctx,
                          const blusys::app_event &event,
                          blusys::feedback_bus *fb,
                          blusys::route_sink *routes)
{
    auto *h = static_cast<demo_handler_ctx *>(ctx);
    if (event.source != blusys::event_source::intent) {
        return;
    }
    if (h->state == nullptr || h->state->slider == nullptr) {
        return;
    }

    switch (blusys::app_event_intent(event)) {
    case blusys::intent::increment: {
        const int32_t cur = blusys::slider_get_value(h->state->slider);
        blusys::slider_set_value(h->state->slider, clamp_slider(cur + kSliderStep));
        blusys::feedback_dispatch(fb, {
            .channel = blusys::feedback_channel::haptic,
            .pattern = blusys::feedback_pattern::click,
            .value   = 1,
            .payload = nullptr,
        });
        break;
    }
    case blusys::intent::decrement: {
        const int32_t cur = blusys::slider_get_value(h->state->slider);
        blusys::slider_set_value(h->state->slider, clamp_slider(cur - kSliderStep));
        blusys::feedback_dispatch(fb, {
            .channel = blusys::feedback_channel::haptic,
            .pattern = blusys::feedback_pattern::click,
            .value   = 1,
            .payload = nullptr,
        });
        break;
    }
    case blusys::intent::confirm: {
        blusys::route_dispatch(routes, blusys::route::show_overlay(1));
        blusys::feedback_dispatch(fb, {
            .channel = blusys::feedback_channel::audio,
            .pattern = blusys::feedback_pattern::confirm,
            .value   = 1,
            .payload = nullptr,
        });
        break;
    }
    default:
        break;
    }
}

// ---- module-level singletons ------------------------------------------------

app_state                   g_state{};
ui_route_sink               g_route_sink{};
logging_feedback_sink       g_feedback_sink{};
blusys::runtime  g_runtime{};
blusys::runtime *g_runtime_ptr = &g_runtime;

void build_demo_screen()
{
    blusys::set_theme(blusys::presets::expressive_dark());

    lv_obj_t *screen = blusys::screen_create({});
    g_state.screen = screen;
    lv_obj_t *col = blusys::col_create(screen, {
        .gap     = blusys::theme().spacing_md,
        .padding = blusys::theme().spacing_lg,
    });

    blusys::label_create(col, {
        .text = "Blusys widget kit",
        .font = blusys::theme().font_title,
    });
    blusys::divider_create(col, {});
    blusys::label_create(col, {
        .text = "Volume",
        .font = blusys::theme().font_body,
    });

    g_state.slider = blusys::slider_create(col, {
        .min       = kSliderMin,
        .max       = kSliderMax,
        .initial   = kSliderInit,
        .on_change = nullptr,
        .user_data = nullptr,
        .disabled  = false,
    });

    lv_obj_t *button_row = blusys::row_create(col, {
        .gap     = blusys::theme().spacing_sm,
        .padding = 0,
    });

    blusys::button_create(button_row, {
        .label   = "Down",
        .variant = blusys::button_variant::secondary,
        .on_press = +[](void *user_data) {
            auto *runtime = static_cast<blusys::runtime *>(user_data);
            runtime->post_intent(blusys::intent::decrement);
        },
        .user_data = g_runtime_ptr,
    });
    blusys::button_create(button_row, {
        .label   = "Up",
        .variant = blusys::button_variant::secondary,
        .on_press = +[](void *user_data) {
            auto *runtime = static_cast<blusys::runtime *>(user_data);
            runtime->post_intent(blusys::intent::increment);
        },
        .user_data = g_runtime_ptr,
    });
    blusys::button_create(button_row, {
        .label   = "OK",
        .variant = blusys::button_variant::primary,
        .on_press = +[](void *user_data) {
            auto *runtime = static_cast<blusys::runtime *>(user_data);
            runtime->post_intent(blusys::intent::confirm);
        },
        .user_data = g_runtime_ptr,
    });

    blusys::label_create(col, {
        .text = "Keys: Arrow keys = focus  |  Space/Enter = press",
        .font = blusys::theme().font_body,
    });

    g_state.toast = blusys::overlay_create(screen, {
        .text        = "Confirmed",
        .duration_ms = 1500,
        .on_hidden   = nullptr,
        .user_data   = nullptr,
    });

    lv_screen_load(screen);
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

    build_demo_screen();

    // ---- keyboard encoder simulation -------------------------------------------
    //
    // Create an LVGL encoder indev fed by keyboard events, wire it to a
    // focus group built from the screen's focusable widgets. This is the
    // host equivalent of framework_ui_basic encoder scenario's hardware encoder bridge.
    SDL_AddEventWatch(sdl_encoder_key_watch, nullptr);

    lv_group_t *encoder_group = blusys::create_encoder_group();
    blusys::auto_focus_screen(g_state.screen, encoder_group);

    lv_indev_t *encoder_indev = lv_indev_create();
    lv_indev_set_type(encoder_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(encoder_indev, encoder_indev_read_cb);
    lv_indev_set_group(encoder_indev, encoder_group);

    // Spine wiring — same order as framework_ui_basic spine scenario.
    g_demo_ctx.state = &g_state;
    g_route_sink.bind(&g_state);
    g_runtime.register_feedback_sink(&g_feedback_sink);

    blusys::runtime_handler handler{};
    handler.context      = &g_demo_ctx;
    handler.on_init      = demo_on_init;
    handler.handle_event = demo_on_event;
    handler.on_tick      = nullptr;
    handler.on_deinit    = nullptr;

    const blusys_err_t init_err = g_runtime.init(&g_route_sink, handler, 10);
    if (init_err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "runtime.init failed: %d", static_cast<int>(init_err));
        return 1;
    }

    BLUSYS_LOGI(kTag, "widget kit demo running — arrow keys=focus, space/enter=press, mouse=click");
    BLUSYS_LOGI(kTag, "initial slider = %ld",
                static_cast<long>(blusys::slider_get_value(g_state.slider)));

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
