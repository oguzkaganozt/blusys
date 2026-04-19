// app_interactive_demo — blusys::app interactive consumer device.
//
// A volume control product demonstrating the full blusys::app model
// with the framework view layer:
//   - action-bound widgets (no callback boilerplate)
//   - page helper (no manual screen/col/focus wiring)
//   - overlay manager (route commands actually show/hide overlays)
//   - binding helpers (no raw lv_label_set_text)
//
// Compare with scripts/host/src/widget_kit_demo.cpp (~400 lines) to
// see the improvement.

#include "blusys/framework/app/app.hpp"
#include "blusys/hal/log.h"

#include <cstdint>
#include <optional>

namespace {

constexpr const char *kTag = "volume_app";

namespace view = blusys;
using blusys::app_ctx;
using blusys::app_fx;

// ---- state ----

struct AppState {
    int32_t  volume = 50;
    bool     muted  = false;
    lv_obj_t *slider     = nullptr;
    lv_obj_t *mute_label = nullptr;
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
            ctx.emit_feedback(blusys::feedback_channel::haptic,
                              blusys::feedback_pattern::click);
        }
        break;

    case Tag::volume_down:
        if (!state.muted && state.volume > 0) {
            state.volume -= 5;
            view::set_value(state.slider, state.volume);
            ctx.emit_feedback(blusys::feedback_channel::haptic,
                              blusys::feedback_pattern::click);
        }
        break;

    case Tag::toggle_mute:
        state.muted = !state.muted;
        view::set_enabled(state.slider, !state.muted);
        view::set_text(state.mute_label, state.muted ? "MUTED" : "Volume");
        BLUSYS_LOGI(kTag, "muted=%s", state.muted ? "true" : "false");
        break;

    case Tag::confirm:
        ctx.fx().nav.show_overlay(1);
        ctx.emit_feedback(blusys::feedback_channel::audio,
                          blusys::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "confirmed — volume=%ld", static_cast<long>(state.volume));
        break;
    }
}

// ---- unified event hook ----

std::optional<Action> on_event(blusys::event event, AppState &state)
{
    (void)state;
    switch (event.source) {
    case blusys::event_source::intent:
        switch (blusys::event_intent(event)) {
        case blusys::intent::increment:
            return Action{.tag = Tag::volume_up};
        case blusys::intent::decrement:
            return Action{.tag = Tag::volume_down};
        case blusys::intent::confirm:
            return Action{.tag = Tag::confirm};
        case blusys::intent::cancel:
            return Action{.tag = Tag::toggle_mute};
        default:
            return std::nullopt;
        }
    default:
        return std::nullopt;
    }
}

// ---- on_init: build the UI ----

void on_init(app_ctx &ctx, app_fx &fx, AppState &state)
{
    (void)fx;
    auto p = view::page_create();

    view::title(p.content, "Blusys Volume Control");
    view::divider(p.content);

    state.mute_label = view::label(p.content, "Volume");

    state.slider = blusys::slider_create(p.content, {
        .min     = 0,
        .max     = 100,
        .initial = state.volume,
    });

    auto *btn_row = view::row(p.content);

    view::button(btn_row, "Down", Action{.tag = Tag::volume_down}, &ctx,
                 blusys::button_variant::secondary);
    view::button(btn_row, "Up", Action{.tag = Tag::volume_up}, &ctx,
                 blusys::button_variant::secondary);
    view::button(btn_row, "Mute", Action{.tag = Tag::toggle_mute}, &ctx,
                 blusys::button_variant::ghost);
    view::button(btn_row, "OK", Action{.tag = Tag::confirm}, &ctx);

    ctx.fx().nav.create_overlay(p.screen, 1, {.text = "Settings saved", .duration_ms = 1500});

    view::page_load(p);

    BLUSYS_LOGI(kTag, "volume control app initialized — volume=%ld",
                static_cast<long>(state.volume));
}

}  // namespace

// ---- app definition ----

static const blusys::app_spec<AppState, Action> spec{
    .initial_state  = {.volume = 50, .muted = false},
    .update         = update,
    .on_init        = on_init,
    .on_tick        = nullptr,
    .on_event       = on_event,
    .tick_period_ms = 10,
};

BLUSYS_APP(spec)
