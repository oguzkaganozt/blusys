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
#include "blusys/framework/ui/theme.hpp"

namespace blusys_app_platform {

void host_init(int w, int h, const char *title)
{
    lv_init();

    lv_display_t *display = lv_sdl_window_create(w, h);
    if (display != nullptr) {
        lv_sdl_window_set_title(display, title);
    }

    (void)lv_sdl_mouse_create();
    (void)lv_sdl_keyboard_create();
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
    blusys::ui::set_theme(blusys::app::presets::dark());
}

void host_set_theme(const void *tokens)
{
    if (tokens != nullptr) {
        blusys::ui::set_theme(*static_cast<const blusys::ui::theme_tokens *>(tokens));
    }
}

}  // namespace blusys_app_platform
