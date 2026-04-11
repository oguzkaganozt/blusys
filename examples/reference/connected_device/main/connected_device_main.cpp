// Connected device example — interactive product with capabilities.
//
// Demonstrates the v7 product path for a connected interactive device:
//   - connectivity capability handles Wi-Fi, SNTP, and mDNS
//   - app shows connection status on an ST7735 display
//   - reducer reacts to both UI intents and capability events

#include "blusys/app/app.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/profiles/st7735.hpp"
#include "blusys/log.h"

#include <cstdint>

namespace {

constexpr const char *kTag = "conn_dev";

namespace view = blusys::app::view;
using blusys::app::app_ctx;

// ---- state ----

struct State {
    bool         connected   = false;
    bool         ready       = false;
    std::int32_t brightness  = 50;
    lv_obj_t    *status_lbl  = nullptr;
    lv_obj_t    *ip_lbl      = nullptr;
    lv_obj_t    *slider      = nullptr;
};

// ---- actions ----

enum class Tag : std::uint8_t {
    wifi_got_ip,
    wifi_disconnected,
    capability_ready,
    brightness_up,
    brightness_down,
    confirm,
};

struct Action {
    Tag tag;
};

// ---- reducer ----

void update(app_ctx &ctx, State &state, const Action &action)
{
    switch (action.tag) {
    case Tag::wifi_got_ip:
        state.connected = true;
        view::set_text(state.status_lbl, "Connected");
        if (auto *conn = ctx.connectivity(); conn != nullptr) {
            view::set_text(state.ip_lbl, conn->ip_info.ip);
        }
        break;

    case Tag::wifi_disconnected:
        state.connected = false;
        view::set_text(state.status_lbl, "Disconnected");
        view::set_text(state.ip_lbl, "---");
        break;

    case Tag::capability_ready:
        state.ready = true;
        view::set_text(state.status_lbl, "Ready");
        BLUSYS_LOGI(kTag, "device ready");
        break;

    case Tag::brightness_up:
        if (state.brightness < 100) {
            state.brightness += 10;
            view::set_value(state.slider, state.brightness);
            ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                              blusys::framework::feedback_pattern::click);
        }
        break;

    case Tag::brightness_down:
        if (state.brightness > 0) {
            state.brightness -= 10;
            view::set_value(state.slider, state.brightness);
            ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                              blusys::framework::feedback_pattern::click);
        }
        break;

    case Tag::confirm:
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "brightness=%ld", static_cast<long>(state.brightness));
        break;
    }
}

// ---- intent map ----

bool map_intent(blusys::framework::intent intent, Action *out)
{
    switch (intent) {
    case blusys::framework::intent::increment:
        *out = Action{.tag = Tag::brightness_up};
        return true;
    case blusys::framework::intent::decrement:
        *out = Action{.tag = Tag::brightness_down};
        return true;
    case blusys::framework::intent::confirm:
        *out = Action{.tag = Tag::confirm};
        return true;
    default:
        return false;
    }
}

// ---- event bridge ----

bool map_event(std::uint32_t id, std::uint32_t /*code*/,
               const void * /*payload*/, Action *out)
{
    using CE = blusys::app::connectivity_event;
    switch (static_cast<CE>(id)) {
    case CE::wifi_got_ip:       *out = Action{.tag = Tag::wifi_got_ip};      return true;
    case CE::wifi_disconnected: *out = Action{.tag = Tag::wifi_disconnected}; return true;
    case CE::capability_ready:      *out = Action{.tag = Tag::capability_ready};     return true;
    default: return false;
    }
}

// ---- on_init: build the UI ----

void on_init(app_ctx &ctx, State &state)
{
    auto p = view::page_create();

    view::title(p.content, "Connected Device");
    view::divider(p.content);

    state.status_lbl = view::label(p.content, "Connecting...");
    state.ip_lbl     = view::label(p.content, "---");

    view::divider(p.content);
    view::label(p.content, "Brightness");

    state.slider = blusys::ui::slider_create(p.content, {
        .min     = 0,
        .max     = 100,
        .initial = state.brightness,
    });

    auto *btn_row = view::row(p.content);
    view::button(btn_row, "-", Action{.tag = Tag::brightness_down}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(btn_row, "+", Action{.tag = Tag::brightness_up}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(btn_row, "OK", Action{.tag = Tag::confirm}, &ctx);

    view::page_load(p);

    BLUSYS_LOGI(kTag, "UI initialized");
}

// ---- capability configuration ----

static blusys::app::connectivity_capability conn{{
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,
    .sntp_server   = "pool.ntp.org",
    .mdns_hostname = "blusys-device",
}};

static blusys::app::capability_list capabilities{&conn};

}  // namespace

// ---- app definition ----

static const blusys::app::app_spec<State, Action> spec{
    .initial_state  = {},
    .update         = update,
    .on_init        = on_init,
    .map_intent     = map_intent,
    .map_event      = map_event,
    .capabilities   = &capabilities,
};

BLUSYS_APP_MAIN_DEVICE(spec, blusys::app::profiles::st7735_160x128())
