/**
 * @file lv_conf.h
 * Host-side LVGL configuration for the Blusys SDL2 harness.
 *
 * This is intentionally minimal: lv_conf_internal.h provides defaults
 * for everything not set here. Only override what the host harness
 * actually needs to differ from the embedded defaults.
 *
 * The matching ESP-IDF managed component pulls lvgl/lvgl ~9.2 (currently
 * resolving to v9.2.2). The CMake here pins the host build to the same
 * upstream tag so behaviour matches the device build.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
 * COLOR / RENDERING
 *====================*/

/* Desktop displays are 32-bit RGBA. */
#define LV_COLOR_DEPTH 32

/*====================
 * MEMORY
 *====================*/

/* Generous memory budget for desktop iteration; the device build
 * sticks with the embedded default (~64 KB). */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (1024U * 1024U)

/*====================
 * OS / TICK / SLEEP
 *====================*/

/* No RTOS on the host — LVGL drives itself off lv_tick_inc / lv_timer_handler. */
#define LV_USE_OS LV_OS_NONE

/* The SDL driver provides its own tick source via SDL_GetTicks; we
 * forward it through `lv_tick_set_cb`. */
#define LV_TICK_CUSTOM 0

/*====================
 * LOGGING
 *====================*/

#define LV_USE_LOG    1
#define LV_LOG_LEVEL  LV_LOG_LEVEL_INFO
#define LV_LOG_PRINTF 1

/*====================
 * FONTS
 *====================*/

/* Match the embedded build's default font. */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*====================
 * INPUT / DEVICES
 *====================*/

/* Enable the SDL display + input drivers. The CMake links lvgl
 * against PkgConfig::SDL2 so the SDL driver sources can find
 * <SDL2/SDL.h>. */
#define LV_USE_SDL              1
#define LV_SDL_INCLUDE_PATH     <SDL2/SDL.h>
#define LV_SDL_RENDER_MODE      LV_DISPLAY_RENDER_MODE_DIRECT
#define LV_SDL_BUF_COUNT        1
#define LV_SDL_ACCELERATED      1
#define LV_SDL_FULLSCREEN       0
#define LV_SDL_DIRECT_EXIT      1
#define LV_SDL_MOUSEWHEEL_MODE  LV_SDL_MOUSEWHEEL_MODE_ENCODER

#endif /*LV_CONF_H*/
