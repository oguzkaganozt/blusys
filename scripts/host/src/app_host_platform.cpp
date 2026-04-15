// Host platform helpers for blusys::app interactive entry.
//
// These functions are called by the template run_host() in entry.hpp.
// They isolate all SDL2 and LVGL initialization from product code.

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_keyboard.h"

#include "blusys/framework/ui/style/presets.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/framework/ui/style/theme.hpp"

namespace blusys::platform {

namespace {

// SDL window pixel size = logical resolution × zoom. LVGL layout uses (w,h) only;
// zoom scales the framebuffer for comfortable host viewing without changing
// coordinates or asking products to pick a separate "host profile".
//
// Override: BLUSYS_HOST_ZOOM=1..8 (integer scale). Set to 1 to disable upscaling.
std::uint8_t host_auto_zoom(int w, int h)
{
    const int min_side = std::min(w, h);
    const int max_side = std::max(w, h);
    if (min_side >= 600 || max_side >= 1200) {
        return 1;
    }
    constexpr int kTargetShortPx = 640;
    int           z                = (kTargetShortPx + min_side - 1) / min_side;
    if (z < 2) {
        z = 2;
    }
    if (z > 4) {
        z = 4;
    }
    return static_cast<std::uint8_t>(z);
}

}  // namespace

// ---- framework runtime binding for intent posting ----

static blusys::runtime *g_runtime = nullptr;

void host_set_runtime(blusys::runtime *rt)
{
    g_runtime = rt;
}

void *host_find_pointer_indev()
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
                g_runtime->post_intent(blusys::intent::increment);
            }
        } else if (diff < 0) {
            for (int i = 0; i < -diff; ++i) {
                g_runtime->post_intent(blusys::intent::decrement);
            }
        }

        // Fire confirm on the press edge, not every poll while held.
        if (g_encoder_state.press_edge) {
            g_encoder_state.press_edge = false;
            g_runtime->post_intent(blusys::intent::confirm);
        }

        // Long-press simulation: fire once after holding Enter for kLongPressMs.
        if (g_encoder_state.enter_down && !g_encoder_state.long_press_fired) {
            const uint32_t held_ms = SDL_GetTicks() - g_encoder_state.enter_down_tick;
            if (held_ms >= kLongPressMs) {
                g_encoder_state.long_press_fired = true;
                g_runtime->post_intent(blusys::intent::long_press);
            }
        }

        // Release intent on key-up edge.
        if (!g_encoder_state.enter_down && g_encoder_state.enter_down_tick != 0) {
            g_runtime->post_intent(blusys::intent::release);
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

        std::uint8_t zoom = host_auto_zoom(w, h);
        if (const char *ez = std::getenv("BLUSYS_HOST_ZOOM")) {
            if (ez[0] != '\0') {
                const int z = std::atoi(ez);
                if (z >= 1 && z <= 8) {
                    zoom = static_cast<std::uint8_t>(z);
                }
            }
        }
        lv_sdl_window_set_zoom(display, zoom);
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

void host_set_theme(const void *tokens)
{
    if (tokens != nullptr) {
        blusys::set_theme(*static_cast<const blusys::theme_tokens *>(tokens));
    }
}

}  // namespace blusys::platform
