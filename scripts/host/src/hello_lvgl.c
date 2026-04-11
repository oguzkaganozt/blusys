/*
 * scripts/host/src/hello_lvgl.c — LVGL-only smoke test.
 *
 * Opens a single SDL2 window via LVGL's bundled SDL display driver,
 * draws a centered rounded rectangle using a small inline theme that
 * mirrors the on-device defaults, and runs the LVGL tick + event loop
 * until the SDL window is closed (or until LV_SDL_DIRECT_EXIT triggers
 * an exit).
 *
 * This is the deliverable that proves the host toolchain works
 * end-to-end before any framework C++ widget code is wired into the
 * harness. Subsequent host work will link the framework component
 * against this same CMake project so the widget kit can be iterated
 * on without flashing hardware on every change.
 */

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_keyboard.h"

#define HOR_RES 480
#define VER_RES 320

static void build_demo_screen(void)
{
    lv_obj_t *screen = lv_screen_active();

    /* Surface background — mirrors color_surface from the example
     * theme used by examples/framework_ui_basic. */
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x11141DU), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    /* Title label. */
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Blusys host harness");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF7F9FFU), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    /* Centered rounded card — the canonical "hello LVGL" rectangle
     * used as the canonical “hello LVGL” card on host. */
    lv_obj_t *card = lv_obj_create(screen);
    lv_obj_set_size(card, 240, 120);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x2A62FFU), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 14, 0);

    lv_obj_t *card_label = lv_label_create(card);
    lv_label_set_text(card_label, "hello, LVGL");
    lv_obj_set_style_text_color(card_label, lv_color_hex(0xF7F9FFU), 0);
    lv_obj_center(card_label);

    /* Footer hint. */
    lv_obj_t *footer = lv_label_create(screen);
    lv_label_set_text(footer, "Close the window or press ESC to exit");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x667089U), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -16);
}

int main(void)
{
    lv_init();

    /* Open a single SDL window — the LVGL SDL driver internally
     * registers a display, mouse and keyboard input device. */
    lv_display_t *display = lv_sdl_window_create(HOR_RES, VER_RES);
    if (display == NULL) {
        fprintf(stderr, "lv_sdl_window_create failed\n");
        return 1;
    }
    lv_sdl_window_set_title(display, "Blusys host harness");

    /* Mouse and keyboard input — same indev pattern an embedded build
     * uses for an encoder, just driven by SDL events. */
    (void)lv_sdl_mouse_create();
    (void)lv_sdl_keyboard_create();

    build_demo_screen();

    fprintf(stdout, "blusys host harness running. close the window to exit.\n");

    /* Main loop: drive LVGL ticks + timers until LV_SDL_DIRECT_EXIT
     * fires (the SDL driver calls exit() once the last window is
     * closed when that option is enabled). */
    uint32_t last_ticks = SDL_GetTicks();
    while (1) {
        const uint32_t now = SDL_GetTicks();
        const uint32_t elapsed = now - last_ticks;
        if (elapsed > 0) {
            lv_tick_inc(elapsed);
            last_ticks = now;
        }
        const uint32_t sleep_ms = lv_timer_handler();
        SDL_Delay(sleep_ms < 5 ? 5 : (sleep_ms > 50 ? 50 : sleep_ms));
    }

    return 0;
}
