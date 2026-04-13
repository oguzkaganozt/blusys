#include "core/app_logic.hpp"
#include "blusys/app/capabilities/mqtt_host.hpp"
#include "blusys/app/view/bindings.hpp"
#include "blusys/log.h"

#ifdef ESP_PLATFORM
#include "integration/mqtt_esp_capability.hpp"
#endif

#include <cstdio>
#include <cstring>

namespace mqtt_dashboard {
namespace {

constexpr const char *kTag = "mqtt_dash";

[[nodiscard]] const blusys::app::mqtt_host_status *mqtt_status(blusys::app::app_ctx &ctx)
{
#if !defined(ESP_PLATFORM)
    if (auto *mh = ctx.mqtt_host(); mh != nullptr) {
        return &mh->status();
    }
    return nullptr;
#else
    if (auto *me = mqtt_dashboard::system::mqtt_esp(); me != nullptr) {
        return &me->status();
    }
    (void)ctx;
    return nullptr;
#endif
}

void sync_shell(app_state &state)
{
    if (state.shell_badge != nullptr) {
        const bool ready = state.mqtt_ready;
        blusys::app::view::set_badge_text(state.shell_badge, ready ? "MQTT" : "Init");
        blusys::app::view::set_badge_level(
            state.shell_badge,
            ready ? blusys::ui::badge_level::success : blusys::ui::badge_level::warning);
    }
    if (state.shell_detail != nullptr) {
#if defined(ESP_PLATFORM)
        blusys::app::view::set_text(state.shell_detail, "Device | WiFi+MQTT");
#else
        blusys::app::view::set_text(state.shell_detail, "Host demo | public broker");
#endif
    }
}

void sync_live(blusys::app::app_ctx &ctx, app_state &state)
{
    if (state.status_line == nullptr) {
        return;
    }
    char line[128];
    const blusys::app::mqtt_host_status *st = mqtt_status(ctx);
    if (st != nullptr) {
        std::snprintf(line, sizeof(line), "%s  tx:%lu  rx:%lu",
                      st->connected ? "Online" : "Connecting...",
                      static_cast<unsigned long>(st->publishes_tx),
                      static_cast<unsigned long>(st->messages_rx));
        blusys::app::view::set_text(state.status_line, line);

        char rx[320];
        if (st->last_topic[0] != '\0') {
            std::snprintf(rx, sizeof(rx), "Last: %.100s -> %.100s", st->last_topic, st->last_payload);
        } else {
            std::snprintf(rx, sizeof(rx), "Last: --");
        }
        if (state.last_rx_line != nullptr) {
            blusys::app::view::set_text(state.last_rx_line, rx);
        }

        if (state.metrics_line != nullptr && st->last_error[0] != '\0') {
            blusys::app::view::set_text(state.metrics_line, st->last_error);
        } else if (state.metrics_line != nullptr) {
            blusys::app::view::set_text(state.metrics_line, "");
        }
    } else {
#if !defined(ESP_PLATFORM)
        blusys::app::view::set_text(state.status_line, "MQTT (host only)");
#else
        blusys::app::view::set_text(state.status_line, "WiFi/MQTT starting...");
#endif
    }
}

void sync_all(blusys::app::app_ctx &ctx, app_state &state)
{
    sync_shell(state);
    sync_live(ctx, state);
}

blusys_err_t publish_json(blusys::app::app_ctx &ctx, const char *topic, const char *json)
{
    const std::size_t len = std::strlen(json);
#if !defined(ESP_PLATFORM)
    auto *mh = ctx.mqtt_host();
    if (mh == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return mh->publish(topic, json, len, -1);
#else
    (void)ctx;
    auto *me = mqtt_dashboard::system::mqtt_esp();
    if (me == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return me->publish(topic, json, len, -1);
#endif
}

}  // namespace

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event)
{
    using CET = blusys::app::capability_event_tag;

    switch (event.tag) {
    case action_tag::capability_event:
        switch (event.cap_event.tag) {
        case CET::mqtt_connected:
        case CET::mqtt_disconnected:
        case CET::mqtt_message_received:
        case CET::mqtt_publish_failed:
        case CET::mqtt_error:
        case CET::mqtt_ready:
            state.mqtt_ready = false;
            if (const blusys::app::mqtt_host_status *st = mqtt_status(ctx); st != nullptr) {
                state.mqtt_ready = st->capability_ready;
            }
            sync_all(ctx, state);
            break;
        case CET::diag_snapshot_ready:
        case CET::diagnostics_ready:
        case CET::diag_console_ready:
            if (const auto *d = ctx.diagnostics(); d != nullptr) {
                state.diagnostics = *d;
            }
            sync_all(ctx, state);
            break;
        default:
            break;
        }
        break;

    case action_tag::mqtt_refresh:
        state.mqtt_ready = false;
        if (const blusys::app::mqtt_host_status *st = mqtt_status(ctx); st != nullptr) {
            state.mqtt_ready = st->capability_ready;
        }
        sync_all(ctx, state);
        break;

    case action_tag::publish_ping: {
        static const char kTopic[] = "blusys/demo/cmd";
        const char       *payload  = "{\"op\":\"ping\",\"v\":1}";
        const blusys_err_t e       = publish_json(ctx, kTopic, payload);
        if (e != BLUSYS_OK) {
            BLUSYS_LOGW(kTag, "publish ping failed: %d", static_cast<int>(e));
        }
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
        sync_all(ctx, state);
        break;
    }

    case action_tag::publish_toggle: {
        state.toggle_state = !state.toggle_state;
        char buf[96];
        std::snprintf(buf, sizeof(buf), "{\"op\":\"toggle\",\"on\":%s}",
                      state.toggle_state ? "true" : "false");
        (void)publish_json(ctx, "blusys/demo/cmd", buf);
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::click);
        sync_all(ctx, state);
        break;
    }

    case action_tag::publish_scene: {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "{\"op\":\"scene\",\"id\":%ld}",
                      static_cast<long>(state.scene_id));
        (void)publish_json(ctx, "blusys/demo/cmd", buf);
        state.scene_id = (state.scene_id % 4) + 1;
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
        sync_all(ctx, state);
        break;
    }

    case action_tag::sync_diagnostics:
        if (const auto *d = ctx.diagnostics(); d != nullptr) {
            state.diagnostics = *d;
        }
        sync_all(ctx, state);
        break;

    case action_tag::show_live:
        ctx.services().navigate_to(route_live);
        break;

    case action_tag::show_about:
        ctx.services().navigate_to(route_about);
        break;
    }
}

bool map_intent(blusys::app::app_services &svc, blusys::framework::intent intent, action *out)
{
    (void)svc;
    switch (intent) {
    case blusys::framework::intent::confirm:
        *out = action{.tag = action_tag::show_about};
        return true;
    case blusys::framework::intent::cancel:
        *out = action{.tag = action_tag::show_live};
        return true;
    default:
        return false;
    }
}

}  // namespace mqtt_dashboard
