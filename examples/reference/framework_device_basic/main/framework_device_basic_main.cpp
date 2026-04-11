// framework_device_basic — canonical device quickstart.
//
// The same volume control app as the host interactive demo
// (scripts/host/src/app_interactive_demo.cpp), running on a real
// ESP32 + ST7735 panel via BLUSYS_APP_MAIN_DEVICE. The app code is
// identical — only the entry macro and profile differ.
//
// This validates that the same blusys::app
// model works on host, headless, and device targets.

#include "blusys/app/app.hpp"
#include "blusys/app/profiles/st7735.hpp"
#include "blusys/log.h"

#include <cstdint>

namespace {

constexpr const char *kTag = "volume_app";

namespace view = blusys::app::view;
using blusys::app::app_ctx;

// ---- state ----

struct AppState {
    std::int32_t volume = 50;
    bool         muted  = false;
    lv_obj_t    *slider     = nullptr;
    lv_obj_t    *mute_label = nullptr;
};

// ---- actions ----

enum class Tag : std::uint8_t {
    volume_up,
    volume_down,
    toggle_mute,
    confirm,
};

struct Action {
    Tag tag;
};

// ---- reducer ----

void update(app_ctx &ctx, AppState &state, const Action &action)
{
    switch (action.tag) {
    case Tag::volume_up:
        if (!state.muted && state.volume < 100) {
            state.volume += 5;
            view::set_value(state.slider, state.volume);
            ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                              blusys::framework::feedback_pattern::click);
        }
        break;

    case Tag::volume_down:
        if (!state.muted && state.volume > 0) {
            state.volume -= 5;
            view::set_value(state.slider, state.volume);
            ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                              blusys::framework::feedback_pattern::click);
        }
        break;

    case Tag::toggle_mute:
        state.muted = !state.muted;
        view::set_enabled(state.slider, !state.muted);
        view::set_text(state.mute_label, state.muted ? "MUTED" : "Volume");
        BLUSYS_LOGI(kTag, "muted=%s", state.muted ? "true" : "false");
        break;

    case Tag::confirm:
        ctx.show_overlay(1);
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "confirmed — volume=%ld", static_cast<long>(state.volume));
        break;
    }
}

// ---- intent-to-action map ----

bool map_intent(blusys::framework::intent intent, Action *out)
{
    switch (intent) {
    case blusys::framework::intent::increment:
        *out = Action{.tag = Tag::volume_up};
        return true;
    case blusys::framework::intent::decrement:
        *out = Action{.tag = Tag::volume_down};
        return true;
    case blusys::framework::intent::confirm:
        *out = Action{.tag = Tag::confirm};
        return true;
    case blusys::framework::intent::cancel:
        *out = Action{.tag = Tag::toggle_mute};
        return true;
    default:
        return false;
    }
}

// ---- on_init: build the UI ----

void on_init(app_ctx &ctx, AppState &state)
{
    auto p = view::page_create();

    view::title(p.content, "Blusys Volume Control");
    view::divider(p.content);

    state.mute_label = view::label(p.content, "Volume");

    state.slider = blusys::ui::slider_create(p.content, {
        .min     = 0,
        .max     = 100,
        .initial = state.volume,
    });

    auto *btn_row = view::row(p.content);

    view::button(btn_row, "Down", Action{.tag = Tag::volume_down}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(btn_row, "Up", Action{.tag = Tag::volume_up}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(btn_row, "Mute", Action{.tag = Tag::toggle_mute}, &ctx,
                 blusys::ui::button_variant::ghost);
    view::button(btn_row, "OK", Action{.tag = Tag::confirm}, &ctx);

    view::overlay_create(p.screen, 1,
                         {.text = "Settings saved", .duration_ms = 1500},
                         *ctx.overlay_manager());

    view::page_load(p);

    BLUSYS_LOGI(kTag, "volume control app initialized — volume=%ld",
                static_cast<long>(state.volume));
}

}  // namespace

// ---- app definition ----

static const blusys::app::app_spec<AppState, Action> spec{
    .initial_state  = {.volume = 50, .muted = false},
    .update         = update,
    .on_init        = on_init,
    .on_tick        = nullptr,
    .map_intent     = map_intent,
    .tick_period_ms = 10,
};

BLUSYS_APP_MAIN_DEVICE(spec, blusys::app::profiles::st7735_160x128())
