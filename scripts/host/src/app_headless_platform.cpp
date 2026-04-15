// Headless platform helpers for blusys::app headless entry.
//
// On host, uses SDL_GetTicks/SDL_Delay. On device this would use
// esp_timer_get_time/vTaskDelay.

#include <SDL2/SDL.h>
#include <cstdint>

namespace {

static bool s_sdl_inited = false;

static void ensure_sdl_init()
{
    if (!s_sdl_inited) {
        SDL_Init(SDL_INIT_TIMER);
        s_sdl_inited = true;
    }
}

}  // namespace

namespace blusys::platform {

std::uint32_t headless_get_ticks_ms()
{
    ensure_sdl_init();
    return SDL_GetTicks();
}

void headless_delay(std::uint32_t ms)
{
    SDL_Delay(ms);
}

}  // namespace blusys::platform
