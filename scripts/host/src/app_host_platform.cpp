// Host platform helpers for blusys::app interactive entry.
//
// These functions are called by the template run_host() in entry.hpp.
// They isolate all SDL2 and LVGL initialization from product code.

#include <SDL2/SDL.h>
#include <cstdint>

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_keyboard.h"

#include "blusys/app/theme_presets.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/framework/ui/theme.hpp"

namespace blusys_app_platform {

// ---- framework runtime binding for intent posting ----

static blusys::framework::runtime *g_runtime = nullptr;

void host_set_runtime(blusys::framework::runtime *rt)
{
    g_runtime = rt;
}

// ---- keyboard encoder indev ----

// Accumulated encoder state, written by the SDL event watcher and
// read/cleared by the LVGL encoder read callback.
static struct {
    int      diff       = 0;  // pending rotation ticks (+ = CW/increment, - = CCW/decrement)
    bool     enter_down = false;
    bool     press_edge = false;  // edge-triggered: set on key-down, cleared after read
    uint32_t enter_down_tick = 0; // SDL tick when Enter was pressed (for long-press)
    bool     long_press_fired = false;
} g_encoder_state;

static constexpr uint32_t kLongPressMs = 500;

// SDL event watcher — fires synchronously on every SDL_PushEvent / SDL_PollEvent
// call. Captures arrow keys and Enter to build the encoder state.
static int encoder_event_watch(void * /*user_data*/, SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        switch (event->key.keysym.sym) {
        case SDLK_RIGHT:
        case SDLK_UP:
            g_encoder_state.diff++;
            break;
        case SDLK_LEFT:
        case SDLK_DOWN:
            g_encoder_state.diff--;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            g_encoder_state.enter_down = true;
            g_encoder_state.press_edge = true;
            g_encoder_state.enter_down_tick = SDL_GetTicks();
            g_encoder_state.long_press_fired = false;
            break;
        default:
            break;
        }
    } else if (event->type == SDL_KEYUP) {
        switch (event->key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            g_encoder_state.enter_down = false;
            break;
        default:
            break;
        }
    }
    return 0;  // 0 = pass event through to other handlers
}

// LVGL encoder read callback — drains the pending encoder state and
// posts framework intents to match the device input_bridge behavior.
static void encoder_read_cb(lv_indev_t * /*indev*/, lv_indev_data_t *data)
{
    const int diff = g_encoder_state.diff;
    g_encoder_state.diff = 0;

    data->enc_diff = static_cast<int16_t>(diff);
    data->state    = g_encoder_state.enter_down
                         ? LV_INDEV_STATE_PRESSED
                         : LV_INDEV_STATE_RELEASED;

    // Post framework intents so the app's map_intent can process encoder
    // input through the standard reducer pipeline — matching device behavior.
    if (g_runtime != nullptr) {
        if (diff > 0) {
            for (int i = 0; i < diff; ++i) {
                g_runtime->post_intent(blusys::framework::intent::increment);
            }
        } else if (diff < 0) {
            for (int i = 0; i < -diff; ++i) {
                g_runtime->post_intent(blusys::framework::intent::decrement);
            }
        }

        // Fire confirm on the press edge, not every poll while held.
        if (g_encoder_state.press_edge) {
            g_encoder_state.press_edge = false;
            g_runtime->post_intent(blusys::framework::intent::confirm);
        }

        // Long-press simulation: fire once after holding Enter for kLongPressMs.
        if (g_encoder_state.enter_down && !g_encoder_state.long_press_fired) {
            const uint32_t held_ms = SDL_GetTicks() - g_encoder_state.enter_down_tick;
            if (held_ms >= kLongPressMs) {
                g_encoder_state.long_press_fired = true;
                g_runtime->post_intent(blusys::framework::intent::long_press);
            }
        }

        // Release intent on key-up edge.
        if (!g_encoder_state.enter_down && g_encoder_state.enter_down_tick != 0) {
            g_runtime->post_intent(blusys::framework::intent::release);
            g_encoder_state.enter_down_tick = 0;
        }
    }
}

void host_init(int w, int h, const char *title)
{
    lv_init();

    lv_display_t *display = lv_sdl_window_create(w, h);
    if (display != nullptr) {
        lv_sdl_window_set_title(display, title);
    }

    (void)lv_sdl_mouse_create();
    (void)lv_sdl_keyboard_create();

    // Register keyboard encoder indev so host interactive apps get the
    // same focus navigation as hardware (arrow keys = encoder rotation,
    // Enter = encoder press).
    SDL_AddEventWatch(encoder_event_watch, nullptr);

    lv_indev_t *enc = lv_indev_create();
    lv_indev_set_type(enc, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(enc, encoder_read_cb);
    // The focus group is attached per-screen via page_load / screen_registry.
    // No default group is set here; screens opt in via lv_indev_set_group.
}

std::uint32_t host_get_ticks_ms()
{
    return SDL_GetTicks();
}

void host_tick_inc(std::uint32_t ms)
{
    lv_tick_inc(ms);
}

std::uint32_t host_frame_step()
{
    return lv_timer_handler();
}

void host_frame_delay(std::uint32_t ms)
{
    SDL_Delay(ms);
}

void host_set_default_theme()
{
    blusys::ui::set_theme(blusys::app::presets::expressive_dark());
}

void host_set_theme(const void *tokens)
{
    if (tokens != nullptr) {
        blusys::ui::set_theme(*static_cast<const blusys::ui::theme_tokens *>(tokens));
    }
}

}  // namespace blusys_app_platform
