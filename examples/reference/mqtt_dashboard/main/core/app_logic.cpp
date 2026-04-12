#include "core/app_logic.hpp"
#include "blusys/app/view/bindings.hpp"
#include "blusys/log.h"

#include <cstdio>
#include <cstring>

namespace mqtt_dashboard {
namespace {

constexpr const char *kTag = "mqtt_dash";

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
        blusys::app::view::set_text(state.shell_detail, "Host demo | public broker");
    }
}

void sync_live(blusys::app::app_ctx &ctx, app_state &state)
{
    if (state.status_line == nullptr) {
        return;
    }
    char line[128];
    auto *mh = ctx.mqtt_host();
#if !defined(ESP_PLATFORM)
    if (mh != nullptr) {
        const auto &s = mh->status();
        std::snprintf(line, sizeof(line), "%s  tx:%lu  rx:%lu",
                      s.connected ? "Online" : "Connecting...",
                      static_cast<unsigned long>(s.publishes_tx),
                      static_cast<unsigned long>(s.messages_rx));
        blusys::app::view::set_text(state.status_line, line);

        char rx[320];
        if (s.last_topic[0] != '\0') {
            std::snprintf(rx, sizeof(rx), "Last: %.100s -> %.100s", s.last_topic, s.last_payload);
        } else {
            std::snprintf(rx, sizeof(rx), "Last: --");
        }
        if (state.last_rx_line != nullptr) {
            blusys::app::view::set_text(state.last_rx_line, rx);
        }

        if (state.metrics_line != nullptr && s.last_error[0] != '\0') {
            blusys::app::view::set_text(state.metrics_line, s.last_error);
        } else if (state.metrics_line != nullptr) {
            blusys::app::view::set_text(state.metrics_line, "");
        }
    } else {
        blusys::app::view::set_text(state.status_line, "MQTT (host only)");
    }
#else
    (void)mh;
    blusys::app::view::set_text(state.status_line, "Use blusys host-build for MQTT");
#endif
}

void sync_all(blusys::app::app_ctx &ctx, app_state &state)
{
    sync_shell(state);
    sync_live(ctx, state);
}

blusys_err_t publish_json(blusys::app::app_ctx &ctx, const char *topic, const char *json)
{
    auto *mh = ctx.mqtt_host();
    if (mh == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    const std::size_t len = std::strlen(json);
    return mh->publish(topic, json, len, -1);
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
            if (auto *mh = ctx.mqtt_host(); mh != nullptr) {
                state.mqtt_ready = mh->status().capability_ready;
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
        if (auto *mh = ctx.mqtt_host(); mh != nullptr) {
            state.mqtt_ready = mh->status().capability_ready;
        }
        sync_all(ctx, state);
        break;

    case action_tag::publish_ping: {
#if !defined(ESP_PLATFORM)
        static const char kTopic[] = "blusys/demo/cmd";
        const char      *payload  = "{\"op\":\"ping\",\"v\":1}";
        const blusys_err_t e      = publish_json(ctx, kTopic, payload);
        if (e != BLUSYS_OK) {
            BLUSYS_LOGW(kTag, "publish ping failed: %d", static_cast<int>(e));
        }
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
#endif
        sync_all(ctx, state);
        break;
    }

    case action_tag::publish_toggle: {
#if !defined(ESP_PLATFORM)
        state.toggle_state = !state.toggle_state;
        char buf[96];
        std::snprintf(buf, sizeof(buf), "{\"op\":\"toggle\",\"on\":%s}",
                      state.toggle_state ? "true" : "false");
        (void)publish_json(ctx, "blusys/demo/cmd", buf);
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::click);
#endif
        sync_all(ctx, state);
        break;
    }

    case action_tag::publish_scene: {
#if !defined(ESP_PLATFORM)
        char buf[96];
        std::snprintf(buf, sizeof(buf), "{\"op\":\"scene\",\"id\":%ld}",
                      static_cast<long>(state.scene_id));
        (void)publish_json(ctx, "blusys/demo/cmd", buf);
        state.scene_id = (state.scene_id % 4) + 1;
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
#endif
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
        ctx.navigate_to(route_live);
        break;

    case action_tag::show_about:
        ctx.navigate_to(route_about);
        break;
    }
}

bool map_intent(blusys::framework::intent intent, action *out)
{
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
