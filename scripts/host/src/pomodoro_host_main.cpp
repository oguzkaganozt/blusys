// scripts/host/src/pomodoro_host_main.cpp — Pomodoro timer on PC + SDL2.
//
// Pixel-identical port of examples/oled_encoder_basic/ to the host
// harness.  The SDL window opens at 4× zoom (512×128) so the 128×32
// OLED layout is legible on a desktop monitor.
//
// Keyboard → encoder mapping:
//   ←  arrow  → CCW  (decrease set time, 1 min per key)
//   →  arrow  → CW   (increase set time, 1 min per key)
//   Space / Enter  → PRESS (IDLE: start  |  RUNNING: stop + reset)
//
// Quit: close the window (LV_SDL_DIRECT_EXIT calls exit() automatically).

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"

namespace {

/* ── layout constants (OLED coordinates, same as device build) ────────── */
constexpr int32_t kW           = 128;
constexpr int32_t kH           = 32;
constexpr uint8_t kZoom        = 4;      /* 512×128 px SDL window          */
constexpr int     kDefaultSecs = 25 * 60;
constexpr int     kMinSecs     =  1 * 60;
constexpr int     kMaxSecs     = 60 * 60;
constexpr int     kStepSecs    =  1 * 60;

/* ── app state ────────────────────────────────────────────────────────── */
enum class State : uint8_t { IDLE, RUNNING };

State s_state     = State::IDLE;
int   s_set_secs  = kDefaultSecs;
int   s_remaining = kDefaultSecs;

/* ── LVGL widget handles ───────────────────────────────────────────────── */
struct AppCtx {
    lv_obj_t *arc;
    lv_obj_t *center_dot;
    lv_obj_t *time_label;
} g_ctx{};

/* ── LVGL animation exec callback ────────────────────────────────────── */
static void arc_anim_exec(void *var, int32_t v)
{
    lv_arc_set_value(static_cast<lv_obj_t *>(var), static_cast<int16_t>(v));
}

/* ── dot visibility helper ───────────────────────────────────────────── */
static void dot_draw(lv_obj_t *dot, State state, bool blink)
{
    bool visible = (state == State::IDLE) || blink;
    lv_obj_set_style_bg_opa(dot, visible ? LV_OPA_COVER : LV_OPA_0, 0);
}

/* ── state-only encoder handler (called from SDL event watch) ────────── */
// Only touches s_state / s_set_secs / s_remaining — no LVGL calls here
// because this is invoked from inside lv_timer_handler()'s SDL pump.
// LVGL widget updates happen in the main loop after timer_handler returns.
static int sdl_key_watch(void * /*userdata*/, SDL_Event *ev)
{
    if (ev->type != SDL_KEYDOWN) return 0;

    const SDL_Keycode key = ev->key.keysym.sym;

    if (s_state == State::IDLE) {
        switch (key) {
        case SDLK_RIGHT:
            s_set_secs  = std::min(kMaxSecs, s_set_secs + kStepSecs);
            s_remaining = s_set_secs;
            break;
        case SDLK_LEFT:
            s_set_secs  = std::max(kMinSecs, s_set_secs - kStepSecs);
            s_remaining = s_set_secs;
            break;
        case SDLK_SPACE:
        case SDLK_RETURN:
            std::printf("I (pomodoro) start — %d min\n", s_set_secs / 60);
            s_state = State::RUNNING;
            break;
        default:
            break;
        }
    } else {
        if (key == SDLK_SPACE || key == SDLK_RETURN) {
            std::printf("I (pomodoro) stopped\n");
            s_remaining = s_set_secs;
            s_state     = State::IDLE;
        }
    }
    return 0;  /* non-consuming: LVGL also sees the event */
}

/* ── screen builder (mirrors device build verbatim) ──────────────────── */
static void build_screen()
{
    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, 0);

    /* Arc ring — 28×28, left panel */
    lv_obj_t *arc = lv_arc_create(screen);
    lv_obj_set_size(arc, 28, 28);
    lv_obj_set_pos(arc, 2, 2);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(arc, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(arc, 0, 0);

    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, kDefaultSecs * 100 / kMaxSecs);

    lv_obj_set_style_arc_color(arc, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 1, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);

    lv_obj_set_style_arc_color(arc, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);

    lv_obj_set_style_opa(arc, LV_OPA_0, LV_PART_KNOB);

    /* Cardinal ticks — 12 / 3 / 6 / 9 o'clock */
    static const struct { int x, y, w, h; } kTicks[] = {
        { 15,  0, 2, 1 },
        { 31, 15, 1, 2 },
        { 15, 31, 2, 1 },
        {  0, 15, 1, 2 },
    };
    for (auto &t : kTicks) {
        lv_obj_t *tick = lv_obj_create(screen);
        lv_obj_set_size(tick, t.w, t.h);
        lv_obj_set_pos(tick, t.x, t.y);
        lv_obj_set_style_radius(tick, 0, 0);
        lv_obj_set_style_bg_color(tick, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tick, 0, 0);
        lv_obj_set_style_pad_all(tick, 0, 0);
        lv_obj_remove_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Center dot — 2×2, blinks while RUNNING */
    lv_obj_t *center_dot = lv_obj_create(screen);
    lv_obj_set_size(center_dot, 2, 2);
    lv_obj_set_pos(center_dot, 15, 15);
    lv_obj_set_style_radius(center_dot, 0, 0);
    lv_obj_set_style_bg_color(center_dot, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(center_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(center_dot, 0, 0);
    lv_obj_set_style_pad_all(center_dot, 0, 0);
    lv_obj_remove_flag(center_dot, LV_OBJ_FLAG_SCROLLABLE);

    /* Dotted vertical divider */
    static const int kDivY[] = { 8, 15, 22 };
    for (int dy : kDivY) {
        lv_obj_t *dot = lv_obj_create(screen);
        lv_obj_set_size(dot, 1, 2);
        lv_obj_set_pos(dot, 32, dy);
        lv_obj_set_style_radius(dot, 0, 0);
        lv_obj_set_style_bg_color(dot, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Right panel container — 95×32 at x=33 */
    lv_obj_t *right = lv_obj_create(screen);
    lv_obj_set_size(right, 95, 32);
    lv_obj_set_pos(right, 33, 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_0, 0);
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_pad_all(right, 0, 0);
    lv_obj_remove_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    /* Time label — montserrat_20, centered */
    lv_obj_t *time_label = lv_label_create(right);
    lv_label_set_text(time_label, "[25:00]");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);

    lv_screen_load(screen);

    g_ctx = {arc, center_dot, time_label};
}

}  // namespace

int main(void)
{
    lv_init();

    lv_display_t *display = lv_sdl_window_create(kW, kH);
    if (display == nullptr) {
        std::fprintf(stderr, "lv_sdl_window_create failed\n");
        return 1;
    }
    lv_sdl_window_set_title(display,
        "Blusys Pomodoro (host)  |  \xe2\x86\x90 \xe2\x86\x92 set time  |  Space = start / stop");
    lv_sdl_window_set_zoom(display, kZoom);

    /* Intercept keyboard events before LVGL processes them.
     * sdl_key_watch is non-consuming: it only updates state variables so
     * LVGL's own SDL driver still sees (and ignores) the events. */
    SDL_AddEventWatch(sdl_key_watch, nullptr);

    build_screen();

    std::printf("I (pomodoro) ready — \xe2\x86\x90 / \xe2\x86\x92 to set time, Space to start/stop\n");

    std::uint32_t last_ticks    = SDL_GetTicks();
    std::uint32_t last_tick_ms  = 0;   /* countdown anchor */
    std::uint32_t last_blink_ms = 0;   /* blink anchor     */
    bool          blink_state   = true;
    State         last_state    = State::IDLE;
    int           last_secs     = -1;
    int           last_arc_val  = -1;

    while (true) {
        /* ── drive LVGL ticks + render + SDL events ─────────────────── */
        const std::uint32_t now_ms = SDL_GetTicks();
        const std::uint32_t elapsed = now_ms - last_ticks;
        if (elapsed > 0) {
            lv_tick_inc(elapsed);
            last_ticks = now_ms;
        }
        /* lv_timer_handler pumps SDL events (calls SDL_PollEvent
         * internally), which fires our sdl_key_watch for any keydown
         * that arrived since the last frame. */
        const std::uint32_t sleep_ms = lv_timer_handler();

        /* ── read updated state AFTER sdl_key_watch may have fired ─── */
        const std::uint32_t frame_ms = SDL_GetTicks();

        /* ── countdown (drift-free 1 s ticks) ──────────────────────── */
        if (s_state == State::RUNNING) {
            if (last_tick_ms == 0) last_tick_ms = frame_ms;
            while (frame_ms - last_tick_ms >= 1000u) {
                last_tick_ms += 1000u;
                s_remaining = std::max(0, s_remaining - 1);
                if (s_remaining == 0) {
                    std::printf("I (pomodoro) done\n");
                    s_remaining  = s_set_secs;
                    s_state      = State::IDLE;
                    last_tick_ms = 0;
                    break;
                }
            }
        } else {
            last_tick_ms = 0;
        }

        /* ── blink (500 ms period while RUNNING) ────────────────────── */
        bool force_redraw = false;
        if (s_state == State::RUNNING) {
            if (last_blink_ms == 0) last_blink_ms = frame_ms;
            if (frame_ms - last_blink_ms >= 500u) {
                last_blink_ms += 500u;
                blink_state   = !blink_state;
                force_redraw  = true;
            }
        } else {
            blink_state   = true;
            last_blink_ms = 0;
        }

        /* ── UI update (mirrors app_task on device) ─────────────────── */
        const int display_secs = (s_state == State::RUNNING) ? s_remaining : s_set_secs;
        const int new_arc_val  = (s_state == State::RUNNING)
                                 ? (s_set_secs > 0 ? s_remaining * 100 / s_set_secs : 0)
                                 : (s_set_secs * 100 / kMaxSecs);

        const bool time_changed = (s_state != last_state || display_secs != last_secs || force_redraw);
        const bool arc_changed  = (new_arc_val != last_arc_val || s_state != last_state);

        if (time_changed || arc_changed) {
            const int mins = display_secs / 60;
            const int secs = display_secs % 60;

            lv_label_set_text_fmt(g_ctx.time_label, "[%02d%s%02d]",
                                  mins, blink_state ? ":" : " ", secs);
            dot_draw(g_ctx.center_dot, s_state, blink_state);

            if (arc_changed) {
                if (s_state == State::RUNNING) {
                    lv_anim_del(g_ctx.arc, arc_anim_exec);
                    lv_anim_t a;
                    lv_anim_init(&a);
                    lv_anim_set_var(&a, g_ctx.arc);
                    lv_anim_set_exec_cb(&a, arc_anim_exec);
                    lv_anim_set_values(&a, lv_arc_get_value(g_ctx.arc), new_arc_val);
                    lv_anim_set_duration(&a, 900);
                    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
                    lv_anim_start(&a);
                } else {
                    lv_anim_del(g_ctx.arc, arc_anim_exec);
                    lv_arc_set_value(g_ctx.arc, new_arc_val);
                }
            }

            last_state   = s_state;
            last_secs    = display_secs;
            last_arc_val = new_arc_val;
        }

        SDL_Delay(sleep_ms < 5 ? 5 : (sleep_ms > 33 ? 33 : sleep_ms));
    }

    return 0;
}
