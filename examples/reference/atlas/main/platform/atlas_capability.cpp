#include "platform/atlas_capability.hpp"
#include "blusys/blusys.hpp"


#include "esp_system.h"
#include "nvs_flash.h"

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

// ---- atlas_command helpers ----------------------------------------------

blusys_err_t atlas_command::ack() const
{
    if (cap == nullptr) return BLUSYS_ERR_INVALID_STATE;
    return cap->publish_command_ack(command_id);
}

blusys_err_t atlas_command::fail(const char *error_message) const
{
    if (cap == nullptr) return BLUSYS_ERR_INVALID_STATE;
    return cap->publish_command_fail(command_id, error_message);
}

blusys_err_t atlas_command::result(const char *json, std::size_t len) const
{
    if (cap == nullptr) return BLUSYS_ERR_INVALID_STATE;
    return cap->publish_command_result(command_id, json, len);
}

std::size_t atlas_command::copy_payload(char *dst, std::size_t dst_sz) const
{
    if (dst == nullptr || dst_sz == 0) return 0;
    const std::size_t n = std::min(payload_len, dst_sz - 1U);
    if (payload_json != nullptr && n > 0) {
        std::memcpy(dst, payload_json, n);
    }
    dst[n] = '\0';
    return n;
}

// ---- atlas_capability ---------------------------------------------------

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

    // Register framework defaults. Products can override any of these
    // via on_command() — last-write-wins by type.
    (void)on_command("ping",            &atlas_capability::builtin_ping,            this);
    (void)on_command("reboot",          &atlas_capability::builtin_reboot,          this);
    (void)on_command("factory_reset",   &atlas_capability::builtin_factory_reset,   this);
    (void)on_command("get_diagnostics", &atlas_capability::builtin_get_diagnostics, this);
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

void atlas_capability::poll(std::uint32_t now_ms)
{
    last_poll_now_ms_ = now_ms;

    const bool is_connected = mqtt_ != nullptr && mqtt_->connected();
    if (is_connected && !was_connected_) {
        on_mqtt_connected();
    } else if (!is_connected && was_connected_) {
        post_integration_event(
            static_cast<std::uint32_t>(atlas_event::broker_disconnected));
    }
    was_connected_ = is_connected;

    // Deferred reboot / factory_reset. The delay gives the QoS 1
    // command_ack PUBACK time to land at the broker before the restart.
    // Not a hard guarantee: if the PUBACK is lost, the backend re-
    // dispatches on reconnect and both reboot / factory_reset are
    // idempotent (both lead to a reboot either way).
    if (pending_reboot_mode_ != reboot_mode::none &&
        static_cast<std::int32_t>(now_ms - pending_reboot_at_ms_) >= 0) {
        const reboot_mode mode = pending_reboot_mode_;
        pending_reboot_mode_ = reboot_mode::none;

        if (mode == reboot_mode::factory_reset) {
            BLUSYS_LOGW(kTag, "factory_reset: erasing NVS and rebooting");
            const esp_err_t err = nvs_flash_erase();
            if (err != ESP_OK) {
                BLUSYS_LOGE(kTag, "nvs_flash_erase failed: %d", static_cast<int>(err));
            }
        } else {
            BLUSYS_LOGW(kTag, "reboot: restarting now");
        }
        esp_restart();
    }
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
        atlas_command cmd;
        cmd.cap          = this;
        cmd.payload_json = msg.payload;
        cmd.payload_len  = msg.payload_len;

        const std::size_t id_n = json_extract_str(
            msg.payload, msg.payload_len, "commandId",
            cmd.command_id, sizeof(cmd.command_id));
        const std::size_t ty_n = json_extract_str(
            msg.payload, msg.payload_len, "type",
            cmd.type, sizeof(cmd.type));

        if (id_n == 0) {
            BLUSYS_LOGW(kTag, "command missing commandId — cannot ack");
            post_integration_event(
                static_cast<std::uint32_t>(atlas_event::command_received), 0, &msg);
            return;
        }

        // Ack means "I have a handler for this type, dispatching now" — it
        // is terminal on the backend's delivery lifecycle. So ack only
        // when a registered handler matches. Otherwise hand to the
        // default handler, which by default publishes
        // `command_fail("unknown_type")` and leaves status=failed.
        // Custom default handlers that want to accept unknown types must
        // call `cmd.ack()` themselves before doing work.
        if (ty_n == 0) {
            (void)publish_command_fail(cmd.command_id, "missing_type");
        } else {
            const command_handler_entry *entry = find_command_handler(cmd.type);
            if (entry != nullptr) {
                (void)publish_command_ack(cmd.command_id);
                entry->fn(cmd, entry->user_ctx);
            } else if (default_handler_fn_ != nullptr) {
                default_handler_fn_(cmd, default_handler_ctx_);
            }
        }

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
        // and will post canonical ota_* events the reducer observes. The
        // checksum is forwarded so the service can verify the staged
        // partition before accepting the new image.
        if (ota_ != nullptr) {
            char url[256];
            char sha[72];  // 64 hex + some slack
            const bool have_url = json_extract_str(msg.payload, msg.payload_len,
                                                   "firmwareUrl", url, sizeof(url)) > 0;
            const bool have_sha = json_extract_str(msg.payload, msg.payload_len,
                                                   "checksum", sha, sizeof(sha)) > 0;
            if (have_url) {
                BLUSYS_LOGI(kTag, "ota dispatch -> %s (verify=%s)",
                            url, have_sha ? "yes" : "no");
                (void)ota_->request_update(url, nullptr, have_sha ? sha : nullptr);
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

blusys_err_t atlas_capability::publish_command_ack(const char *command_id)
{
    if (command_id == nullptr || command_id[0] == '\0') {
        return BLUSYS_ERR_INVALID_ARG;
    }
    char buf[128];
    const int n = std::snprintf(buf, sizeof(buf),
        "{\"type\":\"command_ack\",\"commandId\":\"%s\"}", command_id);
    if (n <= 0) return BLUSYS_ERR_INTERNAL;
    return publish_event(buf, static_cast<std::size_t>(n));
}

blusys_err_t atlas_capability::publish_command_fail(const char *command_id,
                                                     const char *error_message)
{
    if (command_id == nullptr || command_id[0] == '\0') {
        return BLUSYS_ERR_INVALID_ARG;
    }
    char buf[256];
    int n;
    if (error_message != nullptr && error_message[0] != '\0') {
        n = std::snprintf(buf, sizeof(buf),
            "{\"type\":\"command_fail\",\"commandId\":\"%s\",\"errorMessage\":\"%s\"}",
            command_id, error_message);
    } else {
        n = std::snprintf(buf, sizeof(buf),
            "{\"type\":\"command_fail\",\"commandId\":\"%s\"}", command_id);
    }
    if (n <= 0) return BLUSYS_ERR_INTERNAL;
    return publish_event(buf, static_cast<std::size_t>(n));
}

blusys_err_t atlas_capability::publish_command_result(const char *command_id,
                                                       const char *json, std::size_t len)
{
    if (command_id == nullptr || command_id[0] == '\0') {
        return BLUSYS_ERR_INVALID_ARG;
    }
    // Envelope:
    //   {"type":"command_result","commandId":"...","result":<json>}
    // `result` may be any JSON value; when omitted/empty we default to
    // an empty object so the backend never has to distinguish absent
    // from null.
    char buf[768];
    int n;
    if (json != nullptr && len > 0) {
        n = std::snprintf(buf, sizeof(buf),
            "{\"type\":\"command_result\",\"commandId\":\"%s\",\"result\":%.*s}",
            command_id, static_cast<int>(len), json);
    } else {
        n = std::snprintf(buf, sizeof(buf),
            "{\"type\":\"command_result\",\"commandId\":\"%s\",\"result\":{}}",
            command_id);
    }
    if (n <= 0 || static_cast<std::size_t>(n) >= sizeof(buf)) {
        BLUSYS_LOGW(kTag, "command_result too large for buffer");
        return BLUSYS_ERR_NO_MEM;
    }
    return publish_event(buf, static_cast<std::size_t>(n));
}

blusys_err_t atlas_capability::on_command(const char *type,
                                          atlas_command_fn fn,
                                          void *user_ctx)
{
    if (type == nullptr || type[0] == '\0' || fn == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    // Replace existing entry with same type key.
    for (auto &entry : command_handlers_) {
        if (entry.in_use && std::strcmp(entry.type, type) == 0) {
            BLUSYS_LOGW(kTag, "command handler for '%s' replaced", type);
            entry.fn       = fn;
            entry.user_ctx = user_ctx;
            return BLUSYS_OK;
        }
    }

    // Otherwise allocate a free slot.
    for (auto &entry : command_handlers_) {
        if (!entry.in_use) {
            std::strncpy(entry.type, type, sizeof(entry.type) - 1);
            entry.type[sizeof(entry.type) - 1] = '\0';
            entry.fn       = fn;
            entry.user_ctx = user_ctx;
            entry.in_use   = true;
            return BLUSYS_OK;
        }
    }

    BLUSYS_LOGE(kTag, "command registry full (%zu slots) — dropped '%s'",
                kMaxCommandHandlers, type);
    return BLUSYS_ERR_NO_MEM;
}

void atlas_capability::set_default_command_handler(atlas_command_fn fn, void *user_ctx)
{
    default_handler_fn_  = (fn != nullptr) ? fn : &atlas_capability::default_unknown_type;
    default_handler_ctx_ = (fn != nullptr) ? user_ctx : nullptr;
}

const atlas_capability::command_handler_entry *
atlas_capability::find_command_handler(const char *type) const
{
    for (const auto &entry : command_handlers_) {
        if (entry.in_use && std::strcmp(entry.type, type) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

std::size_t atlas_capability::list_supported_commands(char *dst, std::size_t dst_sz) const
{
    if (dst == nullptr || dst_sz == 0) return 0;
    std::size_t w = 0;
    auto append = [&](const char *s) {
        while (*s != '\0' && w + 1 < dst_sz) { dst[w++] = *s++; }
    };
    append("[");
    bool first = true;
    for (const auto &entry : command_handlers_) {
        if (!entry.in_use) continue;
        if (!first) append(",");
        first = false;
        append("\"");
        append(entry.type);
        append("\"");
    }
    append("]");
    if (w >= dst_sz) w = dst_sz - 1;
    dst[w] = '\0';
    return w;
}

void atlas_capability::schedule_reboot(reboot_mode mode, std::uint32_t delay_ms)
{
    pending_reboot_mode_  = mode;
    pending_reboot_at_ms_ = last_poll_now_ms_ + delay_ms;
}

// ---- built-in handlers ---------------------------------------------------

void atlas_capability::builtin_ping(const atlas_command &cmd, void * /*user_ctx*/)
{
    // Reply with an empty result so dashboards confirm round-trip.
    (void)cmd.result("{}", 2);
}

void atlas_capability::builtin_reboot(const atlas_command &cmd, void *user_ctx)
{
    auto *self = static_cast<atlas_capability *>(user_ctx);
    self->schedule_reboot(reboot_mode::reboot, 1000);
    BLUSYS_LOGW(kTag, "reboot scheduled in 1000 ms (commandId=%s)", cmd.command_id);
}

void atlas_capability::builtin_factory_reset(const atlas_command &cmd, void *user_ctx)
{
    auto *self = static_cast<atlas_capability *>(user_ctx);
    self->schedule_reboot(reboot_mode::factory_reset, 1000);
    BLUSYS_LOGW(kTag, "factory_reset scheduled in 1000 ms (commandId=%s)", cmd.command_id);
}

void atlas_capability::builtin_get_diagnostics(const atlas_command &cmd, void *user_ctx)
{
    auto *self = static_cast<atlas_capability *>(user_ctx);

    std::size_t heap = 0;
    (void)blusys_system_free_heap_bytes(&heap);

    std::uint64_t uptime_us = 0;
    (void)blusys_system_uptime_us(&uptime_us);
    const std::uint64_t uptime_ms = uptime_us / 1000ULL;

    std::int8_t rssi = 0;
    if (self->cfg_.connectivity != nullptr) {
        rssi = self->cfg_.connectivity->status().wifi_rssi;
    }

    char supported[256];
    (void)self->list_supported_commands(supported, sizeof(supported));

    char json[512];
    const int n = std::snprintf(json, sizeof(json),
        "{\"firmwareVersion\":\"%s\","
         "\"freeHeap\":%lu,"
         "\"rssi\":%d,"
         "\"uptimeMs\":%llu,"
         "\"supportedCommands\":%s}",
        self->cfg_.firmware_version != nullptr ? self->cfg_.firmware_version : "",
        static_cast<unsigned long>(heap),
        static_cast<int>(rssi),
        static_cast<unsigned long long>(uptime_ms),
        supported);
    if (n <= 0 || static_cast<std::size_t>(n) >= sizeof(json)) {
        (void)cmd.fail("diagnostics_encode_failed");
        return;
    }
    (void)cmd.result(json, static_cast<std::size_t>(n));
}

void atlas_capability::default_unknown_type(const atlas_command &cmd, void * /*user_ctx*/)
{
    (void)cmd.fail("unknown_type");
}

}  // namespace atlas_example
