#ifdef ESP_PLATFORM

#include "integration/mqtt_esp_capability.hpp"

#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/log.h"
#include "blusys/services/protocol/mqtt.h"

#include "esp_netif.h"

#include <cstring>

namespace mqtt_dashboard::system {

namespace {

constexpr const char *kTag = "mqtt_dash_esp";

mqtt_esp_capability *g_mqtt_esp = nullptr;

static void copy_trunc(char *dst, std::size_t dst_sz, const char *src, std::size_t src_len)
{
    if (dst == nullptr || dst_sz == 0) {
        return;
    }
    const std::size_t n = std::min(dst_sz - 1U, src_len);
    if (src != nullptr && n > 0) {
        std::memcpy(dst, src, n);
    }
    dst[n] = '\0';
}

}  // namespace

mqtt_esp_capability *mqtt_esp()
{
    return g_mqtt_esp;
}

void mqtt_esp_capability::message_trampoline(const char *topic, const std::uint8_t *payload,
                                             std::size_t len, void *user_ctx)
{
    auto *self = static_cast<mqtt_esp_capability *>(user_ctx);
    if (self == nullptr) {
        return;
    }
    self->enqueue_incoming(topic, payload, len);
}

void mqtt_esp_capability::enqueue_incoming(const char *topic, const std::uint8_t *payload,
                                          std::size_t len)
{
    pending_in p{};
    if (topic != nullptr) {
        p.topic = topic;
    }
    if (payload != nullptr && len > 0) {
        p.payload.assign(reinterpret_cast<const char *>(payload), len);
    }
    std::lock_guard<std::mutex> lock(incoming_mtx_);
    if (incoming_.size() >= 32U) {
        incoming_.erase(incoming_.begin());
    }
    incoming_.push_back(std::move(p));
}

bool mqtt_esp_capability::sta_has_ip() const
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == nullptr) {
        return false;
    }
    esp_netif_ip_info_t ip{};
    if (esp_netif_get_ip_info(netif, &ip) != ESP_OK) {
        return false;
    }
    return ip.ip.addr != 0;
}

mqtt_esp_capability::mqtt_esp_capability(const mqtt_esp_config &cfg) : cfg_(cfg) {}

blusys_err_t mqtt_esp_capability::start(blusys::framework::runtime &rt)
{
    rt_              = &rt;
    phase_           = phase::wait_sta_ip;
    connect_attempted_ = false;
    status_          = {};
    g_mqtt_esp       = this;
    return BLUSYS_OK;
}

void mqtt_esp_capability::try_start_mqtt_after_ip()
{
    if (connect_attempted_ || mqtt_ != nullptr) {
        return;
    }
    connect_attempted_ = true;
    phase_             = phase::mqtt_connecting;

    blusys_mqtt_config_t mcfg{};
    mcfg.broker_url = cfg_.broker_url;
    mcfg.timeout_ms = cfg_.mqtt_connect_timeout_ms;
    mcfg.message_cb = &mqtt_esp_capability::message_trampoline;
    mcfg.user_ctx   = this;

    blusys_err_t err = blusys_mqtt_open(&mcfg, &mqtt_);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "mqtt_open failed: %d", static_cast<int>(err));
        copy_trunc(status_.last_error, sizeof(status_.last_error), "mqtt_open failed",
                   std::strlen("mqtt_open failed"));
        post_integration_event(static_cast<std::uint32_t>(blusys::app::mqtt_host_event::error));
        phase_ = phase::failed;
        return;
    }

    err = blusys_mqtt_connect(mqtt_);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "mqtt_connect failed: %d", static_cast<int>(err));
        copy_trunc(status_.last_error, sizeof(status_.last_error), "mqtt_connect failed",
                   std::strlen("mqtt_connect failed"));
        post_integration_event(static_cast<std::uint32_t>(blusys::app::mqtt_host_event::error));
        blusys_mqtt_close(mqtt_);
        mqtt_ = nullptr;
        phase_ = phase::failed;
        return;
    }

    if (cfg_.subscribe_pattern != nullptr && cfg_.subscribe_pattern[0] != '\0') {
        err = blusys_mqtt_subscribe(mqtt_, cfg_.subscribe_pattern, BLUSYS_MQTT_QOS_1);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGW(kTag, "mqtt_subscribe failed: %d", static_cast<int>(err));
        }
    }

    status_.connected        = true;
    status_.capability_ready = true;
    status_.last_error[0]    = '\0';
    post_integration_event(static_cast<std::uint32_t>(blusys::app::mqtt_host_event::connected));
    post_integration_event(static_cast<std::uint32_t>(blusys::app::mqtt_host_event::capability_ready));
    phase_ = phase::running;
    BLUSYS_LOGI(kTag, "MQTT session up");
}

void mqtt_esp_capability::flush_pending_incoming()
{
    std::vector<pending_in> batch;
    {
        std::lock_guard<std::mutex> lock(incoming_mtx_);
        batch.swap(incoming_);
    }
    for (const pending_in &p : batch) {
        copy_trunc(status_.last_topic, sizeof(status_.last_topic), p.topic.c_str(), p.topic.size());
        copy_trunc(status_.last_payload, sizeof(status_.last_payload), p.payload.c_str(),
                   p.payload.size());
        status_.messages_rx++;
        post_integration_event(static_cast<std::uint32_t>(blusys::app::mqtt_host_event::message_received));
    }
}

void mqtt_esp_capability::poll(std::uint32_t /*now_ms*/)
{
    if (phase_ == phase::wait_sta_ip) {
        if (sta_has_ip()) {
            try_start_mqtt_after_ip();
        }
        return;
    }
    if (phase_ == phase::running) {
        flush_pending_incoming();
    }
}

void mqtt_esp_capability::stop()
{
    if (mqtt_ != nullptr) {
        blusys_mqtt_disconnect(mqtt_);
        blusys_mqtt_close(mqtt_);
        mqtt_ = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(incoming_mtx_);
        incoming_.clear();
    }
    status_    = {};
    phase_     = phase::wait_sta_ip;
    connect_attempted_ = false;
    rt_        = nullptr;
    if (g_mqtt_esp == this) {
        g_mqtt_esp = nullptr;
    }
}

blusys_err_t mqtt_esp_capability::publish(const char *topic, const void *payload, std::size_t len,
                                           int qos)
{
    if ((topic == nullptr) || (mqtt_ == nullptr) || !blusys_mqtt_is_connected(mqtt_)) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    const blusys_mqtt_qos_t q =
        qos >= 0 ? static_cast<blusys_mqtt_qos_t>(qos)
                 : (cfg_.qos > 0 ? static_cast<blusys_mqtt_qos_t>(cfg_.qos) : BLUSYS_MQTT_QOS_1);
    blusys_err_t err = blusys_mqtt_publish(
        mqtt_, topic, static_cast<const std::uint8_t *>(payload), len, q);
    if (err == BLUSYS_OK) {
        status_.publishes_tx++;
        return BLUSYS_OK;
    }
    copy_trunc(status_.last_error, sizeof(status_.last_error), "publish failed",
               std::strlen("publish failed"));
    post_integration_event(static_cast<std::uint32_t>(blusys::app::mqtt_host_event::publish_failed));
    return err;
}

}  // namespace mqtt_dashboard::system

#endif  // ESP_PLATFORM
