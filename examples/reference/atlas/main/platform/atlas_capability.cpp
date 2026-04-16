#include "platform/atlas_capability.hpp"

#include "blusys/hal/log.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace atlas_example {

namespace {

constexpr const char *kTag = "atlas_proto";

// Extract `"<key>":"<value>"` from a flat JSON object without pulling in
// a full parser. Sufficient for Atlas announcements whose keys never
// contain quotes. Returns bytes written to `dst` (excl. nul), or 0 when
// the key was not found.
std::size_t json_extract_str(const char *json, std::size_t len,
                             const char *key, char *dst, std::size_t dst_sz)
{
    if (dst_sz == 0 || json == nullptr) return 0;
    dst[0] = '\0';

    char needle[64];
    const int nn = std::snprintf(needle, sizeof(needle), "\"%s\"", key);
    if (nn <= 0) return 0;

    const char *end = json + len;
    const char *p   = std::strstr(json, needle);
    if (p == nullptr || p >= end) return 0;
    p += nn;
    while (p < end && (*p == ' ' || *p == '\t' || *p == ':')) ++p;
    if (p >= end || *p != '"') return 0;
    ++p;
    const char *start = p;
    while (p < end && *p != '"') ++p;
    if (p >= end) return 0;
    const std::size_t n =
        std::min(static_cast<std::size_t>(p - start), dst_sz - 1U);
    std::memcpy(dst, start, n);
    dst[n] = '\0';
    return n;
}

}  // namespace

atlas_capability::atlas_capability(const atlas_config &cfg,
                                   blusys::mqtt_capability *mqtt,
                                   blusys::ota_capability  *ota)
    : cfg_(cfg), mqtt_(mqtt), ota_(ota)
{
    const char *id = cfg_.device_id != nullptr ? cfg_.device_id : "unknown";
    std::snprintf(topic_up_state_,     sizeof(topic_up_state_),     "atlas/%s/up/state",     id);
    std::snprintf(topic_up_heartbeat_, sizeof(topic_up_heartbeat_), "atlas/%s/up/heartbeat", id);
    std::snprintf(topic_up_event_,     sizeof(topic_up_event_),     "atlas/%s/up/event",     id);
    std::snprintf(topic_down_command_, sizeof(topic_down_command_), "atlas/%s/down/command", id);
    std::snprintf(topic_down_state_,   sizeof(topic_down_state_),   "atlas/%s/down/state",   id);
    std::snprintf(topic_down_ota_,     sizeof(topic_down_ota_),     "atlas/%s/down/ota",     id);
}

blusys_err_t atlas_capability::start(blusys::runtime &rt)
{
    if (cfg_.device_id == nullptr || mqtt_ == nullptr) {
        BLUSYS_LOGE(kTag, "atlas_capability requires device_id and mqtt_capability*");
        return BLUSYS_ERR_INVALID_ARG;
    }
    rt_ = &rt;
    mqtt_->set_message_handler(&atlas_capability::on_mqtt_message, this);
    BLUSYS_LOGI(kTag, "ready (device=%s)", cfg_.device_id);
    return BLUSYS_OK;
}

void atlas_capability::stop()
{
    if (mqtt_ != nullptr) {
        mqtt_->set_message_handler(nullptr, nullptr);
    }
    rt_            = nullptr;
    was_connected_ = false;
}

void atlas_capability::poll(std::uint32_t /*now_ms*/)
{
    const bool is_connected = mqtt_ != nullptr && mqtt_->connected();
    if (is_connected && !was_connected_) {
        on_mqtt_connected();
    } else if (!is_connected && was_connected_) {
        post_integration_event(
            static_cast<std::uint32_t>(atlas_event::broker_disconnected));
    }
    was_connected_ = is_connected;
}

void atlas_capability::on_mqtt_connected()
{
    // Rising edge of mqtt_->connected(); runs exactly once per session.
    post_integration_event(static_cast<std::uint32_t>(atlas_event::broker_connected));

    // Subscribe at runtime — keeps device_id out of the mqtt_config at
    // compile time. Alternative: populate mqtt_config.auto_subscribe_topics
    // in the owner and let mqtt_capability re-subscribe automatically.
    for (const char *t : {topic_down_command_, topic_down_state_, topic_down_ota_}) {
        const blusys_err_t sr = mqtt_->subscribe(t, 1);
        if (sr != BLUSYS_OK) {
            BLUSYS_LOGW(kTag, "subscribe %s: %s", t, blusys_err_string(sr));
        }
    }

    // Retained "online" — the backend latches this via the EMQX
    // client.connected webhook + retained state fetch.
    char payload[256];
    const int n = std::snprintf(payload, sizeof(payload),
        "{\"online\":true,\"firmwareVersion\":\"%s\"}",
        cfg_.firmware_version != nullptr ? cfg_.firmware_version : "");
    if (n > 0) {
        (void)publish_state(payload, static_cast<std::size_t>(n));
    }
}

void atlas_capability::on_mqtt_message(const blusys::mqtt_message &msg, void *user_ctx)
{
    static_cast<atlas_capability *>(user_ctx)->handle_mqtt_message(msg);
}

void atlas_capability::handle_mqtt_message(const blusys::mqtt_message &msg)
{
    if (std::strcmp(msg.topic, topic_down_command_) == 0) {
        post_integration_event(
            static_cast<std::uint32_t>(atlas_event::command_received), 0, &msg);
        return;
    }
    if (std::strcmp(msg.topic, topic_down_state_) == 0) {
        post_integration_event(
            static_cast<std::uint32_t>(atlas_event::state_desired), 0, &msg);
        return;
    }
    if (std::strcmp(msg.topic, topic_down_ota_) == 0) {
        post_integration_event(
            static_cast<std::uint32_t>(atlas_event::ota_announcement), 0, &msg);
        // Atlas sends {otaJobId, firmwareUrl, targetVersion, checksum}.
        // Drive ota_capability directly — it owns the download + flash,
        // and will post canonical ota_* events the reducer observes.
        if (ota_ != nullptr) {
            char url[256];
            if (json_extract_str(msg.payload, msg.payload_len, "firmwareUrl",
                                 url, sizeof(url)) > 0) {
                BLUSYS_LOGI(kTag, "ota dispatch -> %s", url);
                (void)ota_->request_update(url, nullptr);
            } else {
                BLUSYS_LOGW(kTag, "ota announcement missing firmwareUrl");
            }
        }
        return;
    }
    BLUSYS_LOGD(kTag, "ignored topic: %s", msg.topic);
}

blusys_err_t atlas_capability::publish_state(const char *json, std::size_t len)
{
    if (mqtt_ == nullptr) return BLUSYS_ERR_INVALID_STATE;
    return mqtt_->publish(topic_up_state_, json, len, 1, /*retain=*/true);
}

blusys_err_t atlas_capability::publish_heartbeat(const char *json, std::size_t len)
{
    if (mqtt_ == nullptr) return BLUSYS_ERR_INVALID_STATE;
    return mqtt_->publish(topic_up_heartbeat_, json, len, 1, /*retain=*/false);
}

blusys_err_t atlas_capability::publish_event(const char *json, std::size_t len)
{
    if (mqtt_ == nullptr) return BLUSYS_ERR_INVALID_STATE;
    return mqtt_->publish(topic_up_event_, json, len, 1, /*retain=*/false);
}

}  // namespace atlas_example
