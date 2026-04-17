#ifdef ESP_PLATFORM

#include "blusys/framework/capabilities/mqtt.hpp"

#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

#include <algorithm>
#include <cstring>

namespace blusys {
namespace {

constexpr const char *kTag = "mqtt";

constexpr std::size_t kMaxPending = 16;

void copy_trunc(char *dst, std::size_t dst_sz, const char *src, std::size_t src_len)
{
    if (dst_sz == 0) return;
    const std::size_t n = (src == nullptr) ? 0 : std::min(src_len, dst_sz - 1U);
    if (n > 0) std::memcpy(dst, src, n);
    dst[n] = '\0';
}

}  // namespace

mqtt_capability::mqtt_capability(const mqtt_config &cfg)
    : cfg_(cfg)
{
}

blusys_err_t mqtt_capability::start(blusys::runtime &rt)
{
    if (cfg_.broker_url == nullptr) {
        BLUSYS_LOGE(kTag, "mqtt_config.broker_url is required");
        return BLUSYS_ERR_INVALID_ARG;
    }
    rt_                = &rt;
    backoff_ms_        = cfg_.reconnect_min_ms;
    next_reconnect_ms_ = 0;
    connected_         = false;
    BLUSYS_LOGI(kTag, "ready — broker=%s client_id=%s",
                cfg_.broker_url, cfg_.client_id != nullptr ? cfg_.client_id : "(auto)");
    return BLUSYS_OK;
}

void mqtt_capability::stop()
{
    close_broker();
    rt_ = nullptr;
    {
        std::lock_guard<std::mutex> lock(pending_mtx_);
        pending_.clear();
    }
    drained_.clear();
    pending_connected_.store(false);
    pending_disconnected_.store(false);
    pending_error_.store(false);
    connected_     = false;
    network_ready_ = false;
}

void mqtt_capability::poll(std::uint32_t now_ms)
{
    // ---- 1. Drain connection-state transitions from the MQTT task. ----

    if (pending_connected_.exchange(false)) {
        connected_               = true;
        status_.connected        = true;
        status_.capability_ready = true;
        backoff_ms_              = cfg_.reconnect_min_ms;
        issue_auto_subscribes();
        post_ev(mqtt_event::connected);
        post_ev(mqtt_event::capability_ready);
    }
    if (pending_disconnected_.exchange(false)) {
        connected_               = false;
        status_.connected        = false;
        status_.capability_ready = false;
        post_ev(mqtt_event::disconnected);
    }
    if (pending_error_.exchange(false)) {
        post_ev(mqtt_event::error);
    }

    // ---- 2. Drain inbound messages. ----

    drained_.clear();
    {
        std::lock_guard<std::mutex> lock(pending_mtx_);
        drained_.swap(pending_);
    }
    for (const auto &m : drained_) {
        status_.messages_rx++;
        if (handler_ != nullptr) {
            handler_(m, handler_ctx_);
        }
        post_ev(mqtt_event::message_received, &m);
    }

    // ---- 3. Lazy open / reconnect. ----

    if (!connected_ && network_ready_ && now_ms >= next_reconnect_ms_) {
        const blusys_err_t err = open_broker();
        if (err != BLUSYS_OK) {
            copy_trunc(status_.last_error, sizeof(status_.last_error),
                       blusys_err_string(err),
                       std::strlen(blusys_err_string(err)));
            next_reconnect_ms_ = now_ms + backoff_ms_;
            backoff_ms_        = std::min(backoff_ms_ * 2U, cfg_.reconnect_max_ms);
            post_ev(mqtt_event::error);
        }
        // On success, state_cb → pending_connected_ will be picked up next poll().
    }
}

blusys_err_t mqtt_capability::open_broker()
{
    if (mqtt_handle_ != nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_mqtt_config_t mc{};
    mc.broker_url      = cfg_.broker_url;
    mc.username        = cfg_.username;
    mc.password        = cfg_.password;
    mc.client_id       = cfg_.client_id;
    mc.server_cert_pem = cfg_.server_cert_pem;
    mc.timeout_ms      = cfg_.connect_timeout_ms;
    mc.message_cb      = &mqtt_capability::mqtt_cb;
    mc.state_cb        = &mqtt_capability::state_cb;
    mc.user_ctx        = this;
    mc.will_topic       = cfg_.will_topic;
    mc.will_payload     = cfg_.will_payload;
    mc.will_payload_len = cfg_.will_payload_len;
    mc.will_qos         = static_cast<blusys_mqtt_qos_t>(cfg_.will_qos);
    mc.will_retain      = cfg_.will_retain;

    blusys_mqtt_t *handle = nullptr;
    blusys_err_t err = blusys_mqtt_open(&mc, &handle);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "open failed: %s", blusys_err_string(err));
        return err;
    }

    err = blusys_mqtt_connect(handle);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGW(kTag, "connect failed: %s", blusys_err_string(err));
        blusys_mqtt_close(handle);
        return err;
    }
    mqtt_handle_ = handle;
    return BLUSYS_OK;
}

void mqtt_capability::close_broker()
{
    if (mqtt_handle_ == nullptr) return;
    (void)blusys_mqtt_disconnect(mqtt_handle_);
    (void)blusys_mqtt_close(mqtt_handle_);
    mqtt_handle_             = nullptr;
    // state_cb would fire on graceful disconnect; set the pending flag
    // directly here so `poll()` observes the transition even if the
    // capability is being torn down and won't tick again.
    pending_disconnected_.store(true);
}

void mqtt_capability::issue_auto_subscribes()
{
    const int qos = cfg_.default_qos > 0 ? cfg_.default_qos : 1;
    for (const char *t : cfg_.auto_subscribe_topics) {
        if (t == nullptr || t[0] == '\0') break;
        (void)subscribe(t, qos);
    }
}

void mqtt_capability::mqtt_cb(const char *topic, const std::uint8_t *payload,
                              std::size_t payload_len, void *user_ctx)
{
    auto *self = static_cast<mqtt_capability *>(user_ctx);
    if (self == nullptr || topic == nullptr) return;

    std::lock_guard<std::mutex> lock(self->pending_mtx_);
    if (self->pending_.size() >= kMaxPending) {
        self->pending_.erase(self->pending_.begin());
    }
    mqtt_message m{};
    copy_trunc(m.topic, sizeof(m.topic), topic, std::strlen(topic));
    const std::size_t cap = sizeof(m.payload) - 1U;
    const std::size_t n   = std::min(payload_len, cap);
    if (payload != nullptr && n > 0) {
        std::memcpy(m.payload, payload, n);
    }
    m.payload[n]  = '\0';
    m.payload_len = n;
    self->pending_.push_back(m);
}

void mqtt_capability::state_cb(blusys_mqtt_state_t state, void *user_ctx)
{
    auto *self = static_cast<mqtt_capability *>(user_ctx);
    if (self == nullptr) return;
    switch (state) {
    case BLUSYS_MQTT_STATE_CONNECTED:
        self->pending_connected_.store(true);
        break;
    case BLUSYS_MQTT_STATE_DISCONNECTED:
        self->pending_disconnected_.store(true);
        break;
    case BLUSYS_MQTT_STATE_ERROR:
        self->pending_error_.store(true);
        break;
    }
}

blusys_err_t mqtt_capability::publish(const char *topic,
                                      const void *payload, std::size_t len,
                                      int qos, bool retain)
{
    if (mqtt_handle_ == nullptr || !connected_) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    const int q = qos >= 0 ? qos : (cfg_.default_qos > 0 ? cfg_.default_qos : 1);
    const blusys_err_t err = blusys_mqtt_publish(
        mqtt_handle_, topic,
        static_cast<const std::uint8_t *>(payload), len,
        static_cast<blusys_mqtt_qos_t>(q), retain);
    if (err == BLUSYS_OK) {
        status_.publishes_tx++;
    } else {
        post_ev(mqtt_event::publish_failed);
    }
    return err;
}

blusys_err_t mqtt_capability::subscribe(const char *topic_filter, int qos)
{
    if (mqtt_handle_ == nullptr || !connected_) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    const int q = qos >= 0 ? qos : (cfg_.default_qos > 0 ? cfg_.default_qos : 1);
    return blusys_mqtt_subscribe(mqtt_handle_, topic_filter,
                                 static_cast<blusys_mqtt_qos_t>(q));
}

blusys_err_t mqtt_capability::unsubscribe(const char *topic_filter)
{
    if (mqtt_handle_ == nullptr || !connected_) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return blusys_mqtt_unsubscribe(mqtt_handle_, topic_filter);
}

void mqtt_capability::set_message_handler(mqtt_message_handler_fn handler, void *user_ctx)
{
    handler_     = handler;
    handler_ctx_ = user_ctx;
}

}  // namespace blusys

#else  // !ESP_PLATFORM — host stub (use mqtt_host_capability instead).

#include "blusys/framework/capabilities/mqtt.hpp"
#include "blusys/hal/log.h"

namespace blusys {

mqtt_capability::mqtt_capability(const mqtt_config &cfg) : cfg_(cfg) {}

blusys_err_t mqtt_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    BLUSYS_LOGW("mqtt",
                "device mqtt_capability is ESP-IDF only; use mqtt_host_capability on host");
    return BLUSYS_ERR_NOT_SUPPORTED;
}

void mqtt_capability::stop() { rt_ = nullptr; }
void mqtt_capability::poll(std::uint32_t) {}

blusys_err_t mqtt_capability::publish(const char *, const void *, std::size_t, int, bool)
{ return BLUSYS_ERR_NOT_SUPPORTED; }

blusys_err_t mqtt_capability::subscribe(const char *, int)
{ return BLUSYS_ERR_NOT_SUPPORTED; }

blusys_err_t mqtt_capability::unsubscribe(const char *)
{ return BLUSYS_ERR_NOT_SUPPORTED; }

void mqtt_capability::set_message_handler(mqtt_message_handler_fn handler, void *user_ctx)
{
    handler_     = handler;
    handler_ctx_ = user_ctx;
}

blusys_err_t mqtt_capability::open_broker() { return BLUSYS_ERR_NOT_SUPPORTED; }
void mqtt_capability::close_broker() {}
void mqtt_capability::issue_auto_subscribes() {}

}  // namespace blusys

#endif  // ESP_PLATFORM
