// Host (SDL) MQTT capability — libmosquitto. Linked only from PC host example CMake graphs.

#include "blusys/app/capabilities/mqtt_host.hpp"

#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

#include <mosquitto.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace blusys::app {
namespace {

constexpr const char *kTag = "mqtt_host";

constexpr std::size_t kMaxPending = 64;

static void copy_trunc(char *dst, std::size_t dst_sz, const char *src, std::size_t src_len)
{
    if (dst_sz == 0) {
        return;
    }
    const std::size_t n = std::min(src_len, dst_sz - 1U);
    if (src != nullptr && n > 0) {
        std::memcpy(dst, src, n);
    }
    dst[n] = '\0';
}

static bool parse_broker_url(const char *url, char *host_out, std::size_t host_sz, int *port_out)
{
    if ((url == nullptr) || (host_out == nullptr) || (port_out == nullptr) || (host_sz < 2)) {
        return false;
    }
    *port_out = 1883;
    if (std::strncmp(url, "mqtt://", 7) != 0) {
        return false;
    }
    const char *p = url + 7;
    const char *colon = std::strchr(p, ':');
    const char *slash = std::strchr(p, '/');
    if (colon != nullptr && (slash == nullptr || colon < slash)) {
        const std::size_t hlen = static_cast<std::size_t>(colon - p);
        if (hlen >= host_sz) {
            return false;
        }
        std::memcpy(host_out, p, hlen);
        host_out[hlen] = '\0';
        int pnum = std::atoi(colon + 1);
        if (pnum > 0 && pnum < 65536) {
            *port_out = pnum;
        }
        return true;
    }
    const char *end = slash != nullptr ? slash : p + std::strlen(p);
    const std::size_t hlen = static_cast<std::size_t>(end - p);
    if (hlen == 0 || hlen >= host_sz) {
        return false;
    }
    std::memcpy(host_out, p, hlen);
    host_out[hlen] = '\0';
    return true;
}

static void on_connect_cb(struct mosquitto *mq, void *userdata, int result)
{
    auto *self = static_cast<mqtt_host_capability *>(userdata);
    if (self == nullptr) {
        return;
    }
    if (result != 0) {
        self->notify_error("MQTT connect refused");
        return;
    }
    const mqtt_host_config &c = self->broker_config();
    const int                 qos = c.qos > 0 ? c.qos : 1;
    for (std::size_t i = 0; i < 8; ++i) {
        const char *sub = c.subscribe_topics[i];
        if (sub == nullptr || sub[0] == '\0') {
            break;
        }
        const int sr = mosquitto_subscribe(mq, nullptr, sub, qos);
        if (sr != MOSQ_ERR_SUCCESS) {
            BLUSYS_LOGW(kTag, "subscribe %s: %s", sub, mosquitto_strerror(sr));
        }
    }
    self->notify_connected();
}

static void on_disconnect_cb(struct mosquitto * /*mq*/, void *userdata, int /*rc*/)
{
    auto *self = static_cast<mqtt_host_capability *>(userdata);
    if (self != nullptr) {
        self->notify_disconnected();
    }
}

static void on_message_cb(struct mosquitto * /*mq*/, void *userdata,
                          const struct mosquitto_message *msg)
{
    if ((userdata == nullptr) || (msg == nullptr) || (msg->topic == nullptr)) {
        return;
    }
    auto *self = static_cast<mqtt_host_capability *>(userdata);
    const std::size_t plen =
        msg->payloadlen > 0 ? static_cast<std::size_t>(msg->payloadlen) : 0U;
    self->notify_message(msg->topic, std::strlen(msg->topic), msg->payload, plen);
}

}  // namespace

mqtt_host_capability::mqtt_host_capability(const mqtt_host_config &cfg)
    : cfg_(cfg)
{
}

blusys_err_t mqtt_host_capability::start(blusys::framework::runtime &rt)
{
    if (cfg_.broker_url == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    rt_ = &rt;

    if (!started_lib_) {
        const int li = mosquitto_lib_init();
        if (li != 0) {
            BLUSYS_LOGE(kTag, "mosquitto_lib_init failed: %d", li);
            return BLUSYS_ERR_IO;
        }
        started_lib_ = true;
    }

    char idbuf[64];
    const char *cid = cfg_.client_id;
    if (cid == nullptr || cid[0] == '\0') {
        std::snprintf(idbuf, sizeof(idbuf), "blusys_%ld", static_cast<long>(getpid()));
        cid = idbuf;
    }

    auto *mq = mosquitto_new(cid, true, this);
    if (mq == nullptr) {
        BLUSYS_LOGE(kTag, "mosquitto_new failed");
        return BLUSYS_ERR_NO_MEM;
    }
    mosq_ = mq;

    mosquitto_connect_callback_set(mq, on_connect_cb);
    mosquitto_disconnect_callback_set(mq, on_disconnect_cb);
    mosquitto_message_callback_set(mq, on_message_cb);

    char host[128];
    int  port = 1883;
    if (!parse_broker_url(cfg_.broker_url, host, sizeof(host), &port)) {
        BLUSYS_LOGE(kTag, "invalid broker_url");
        mosquitto_destroy(mq);
        mosq_ = nullptr;
        return BLUSYS_ERR_INVALID_ARG;
    }

    const int cr = mosquitto_connect_async(
        mq, host, port, cfg_.keepalive_sec > 0 ? cfg_.keepalive_sec : 60);
    if (cr != MOSQ_ERR_SUCCESS) {
        BLUSYS_LOGE(kTag, "mosquitto_connect_async failed: %s", mosquitto_strerror(cr));
        mosquitto_destroy(mq);
        mosq_ = nullptr;
        return BLUSYS_ERR_IO;
    }

    const int lr = mosquitto_loop_start(mq);
    if (lr != MOSQ_ERR_SUCCESS) {
        BLUSYS_LOGE(kTag, "mosquitto_loop_start failed: %s", mosquitto_strerror(lr));
        mosquitto_disconnect(mq);
        mosquitto_destroy(mq);
        mosq_ = nullptr;
        return BLUSYS_ERR_IO;
    }

    BLUSYS_LOGI(kTag, "connecting to %s:%d", host, port);
    return BLUSYS_OK;
}

void mqtt_host_capability::stop()
{
    if (mosq_ != nullptr) {
        auto *mq = static_cast<struct mosquitto *>(mosq_);
        (void)mosquitto_loop_stop(mq, false);
        (void)mosquitto_disconnect(mq);
        mosquitto_destroy(mq);
        mosq_ = nullptr;
    }
    if (started_lib_) {
        mosquitto_lib_cleanup();
        started_lib_ = false;
    }
    {
        std::lock_guard<std::mutex> lock(pending_mtx_);
        pending_.clear();
    }
    rt_     = nullptr;
    status_ = {};
}

void mqtt_host_capability::poll(std::uint32_t /*now_ms*/)
{
    flush_pending();
}

void mqtt_host_capability::notify_connected()
{
    std::lock_guard<std::mutex> lock(pending_mtx_);
    if (pending_.size() >= kMaxPending) {
        pending_.erase(pending_.begin());
    }
    Pending p{};
    p.ev           = mqtt_host_event::connected;
    p.has_message = false;
    pending_.push_back(p);
}

void mqtt_host_capability::notify_disconnected()
{
    std::lock_guard<std::mutex> lock(pending_mtx_);
    if (pending_.size() >= kMaxPending) {
        pending_.erase(pending_.begin());
    }
    Pending p{};
    p.ev           = mqtt_host_event::disconnected;
    p.has_message = false;
    pending_.push_back(p);
}

void mqtt_host_capability::notify_message(const char *topic, std::size_t topic_len,
                                          const void *payload, std::size_t payload_len)
{
    std::lock_guard<std::mutex> lock(pending_mtx_);
    if (pending_.size() >= kMaxPending) {
        pending_.erase(pending_.begin());
    }
    Pending p{};
    p.ev           = mqtt_host_event::message_received;
    p.has_message = true;
    copy_trunc(p.topic, sizeof(p.topic), topic, topic_len);
    if (payload != nullptr && payload_len > 0) {
        copy_trunc(p.payload, sizeof(p.payload), static_cast<const char *>(payload), payload_len);
        p.payload_len = std::min(payload_len, sizeof(p.payload) - 1U);
    } else {
        p.payload[0]  = '\0';
        p.payload_len = 0;
    }
    pending_.push_back(p);
}

void mqtt_host_capability::notify_error(const char *msg)
{
    if (msg == nullptr) {
        msg = "MQTT error";
    }
    std::lock_guard<std::mutex> lock(pending_mtx_);
    if (pending_.size() >= kMaxPending) {
        pending_.erase(pending_.begin());
    }
    Pending p{};
    p.ev = mqtt_host_event::error;
    copy_trunc(p.payload, sizeof(p.payload), msg, std::strlen(msg));
    p.payload_len = std::min(std::strlen(msg), sizeof(p.payload) - 1U);
    p.has_message = true;
    pending_.push_back(p);
}

void mqtt_host_capability::post_ev(mqtt_host_event ev)
{
    if (rt_ == nullptr) {
        return;
    }
    (void)rt_->post_event(blusys::framework::make_integration_event(
        static_cast<std::uint32_t>(ev), 0, nullptr));
}

void mqtt_host_capability::flush_pending()
{
    std::vector<Pending> batch;
    {
        std::lock_guard<std::mutex> lock(pending_mtx_);
        batch.swap(pending_);
    }
    for (const Pending &it : batch) {
        switch (it.ev) {
        case mqtt_host_event::connected:
            status_.connected = true;
            copy_trunc(status_.last_error, sizeof(status_.last_error), "", 0);
            post_ev(mqtt_host_event::connected);
            status_.capability_ready = true;
            post_ev(mqtt_host_event::capability_ready);
            break;
        case mqtt_host_event::disconnected:
            status_.connected = false;
            status_.capability_ready = false;
            post_ev(mqtt_host_event::disconnected);
            break;
        case mqtt_host_event::message_received:
            if (it.has_message) {
                copy_trunc(status_.last_topic, sizeof(status_.last_topic), it.topic,
                           std::strlen(it.topic));
                copy_trunc(status_.last_payload, sizeof(status_.last_payload), it.payload,
                           it.payload_len);
                status_.messages_rx++;
            }
            post_ev(mqtt_host_event::message_received);
            break;
        case mqtt_host_event::error:
            if (it.has_message) {
                copy_trunc(status_.last_error, sizeof(status_.last_error), it.payload,
                           it.payload_len);
            }
            post_ev(mqtt_host_event::error);
            break;
        case mqtt_host_event::publish_failed:
            if (it.has_message) {
                copy_trunc(status_.last_error, sizeof(status_.last_error), it.payload,
                           it.payload_len);
            }
            post_ev(mqtt_host_event::publish_failed);
            break;
        default:
            break;
        }
    }
}

blusys_err_t mqtt_host_capability::publish(const char *topic, const void *payload, std::size_t len,
                                            int qos)
{
    if ((topic == nullptr) || (mosq_ == nullptr)) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    auto *mq = static_cast<struct mosquitto *>(mosq_);
    const int q = qos >= 0 ? qos : (cfg_.qos > 0 ? cfg_.qos : 1);
    const int r =
        mosquitto_publish(mq, nullptr, topic, static_cast<int>(len),
                          len > 0 ? payload : nullptr, q, false);
    if (r == MOSQ_ERR_SUCCESS) {
        status_.publishes_tx++;
        return BLUSYS_OK;
    }
    const char *err = mosquitto_strerror(r);
    {
        std::lock_guard<std::mutex> lock(pending_mtx_);
        if (pending_.size() >= kMaxPending) {
            pending_.erase(pending_.begin());
        }
        Pending p{};
        p.ev          = mqtt_host_event::publish_failed;
        p.has_message = true;
        copy_trunc(p.payload, sizeof(p.payload), err, std::strlen(err));
        p.payload_len = std::min(std::strlen(err), sizeof(p.payload) - 1U);
        pending_.push_back(p);
    }
    return BLUSYS_ERR_IO;
}

}  // namespace blusys::app
