#pragma once

// Device-side MQTT capability. Wraps the `blusys_mqtt` C service and
// lifts topic traffic into the canonical capability event stream
// (raw IDs 0x08xx band, same as `mqtt_host_capability`).

#include "blusys/framework/capabilities/capability.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace blusys { class runtime; }

namespace blusys {

enum class mqtt_event : std::uint32_t {
    connected         = 0x0800,
    disconnected      = 0x0801,
    message_received  = 0x0802,
    publish_failed    = 0x0803,
    error             = 0x080E,
    capability_ready  = 0x08FF,
};

// Message delivered via integration event payload. Storage lives inside
// the capability's `drained_` buffer — valid from the poll() that drained
// it until the NEXT poll().
struct mqtt_message {
    char        topic[160]   = {};
    char        payload[512] = {};
    std::size_t payload_len  = 0;
};

struct mqtt_status : capability_status_base {
    bool          connected      = false;
    std::uint32_t messages_rx    = 0;
    std::uint32_t publishes_tx   = 0;
    char          last_error[96] = {};
};

using mqtt_message_handler_fn = void (*)(const mqtt_message &msg, void *user_ctx);

struct mqtt_config {
    const char *broker_url        = nullptr;
    const char *client_id         = nullptr;
    const char *username          = nullptr;
    const char *password          = nullptr;
    const char *server_cert_pem   = nullptr;
    int         connect_timeout_ms = 15000;

    const char *auto_subscribe_topics[8] = {};
    int         default_qos              = 1;

    std::uint32_t reconnect_min_ms = 2000;
    std::uint32_t reconnect_max_ms = 60000;

    const char   *will_topic       = nullptr;
    const char   *will_payload     = nullptr;
    std::size_t   will_payload_len = 0;
    int           will_qos         = 1;
    bool          will_retain      = false;
};

class mqtt_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::mqtt;

    explicit mqtt_capability(const mqtt_config &cfg);
    mqtt_capability(const mqtt_capability &)            = delete;
    mqtt_capability &operator=(const mqtt_capability &) = delete;

    [[nodiscard]] capability_kind kind() const override { return capability_kind::mqtt; }

    blusys_err_t start(blusys::runtime &rt) override;
    void         poll(std::uint32_t now_ms) override;
    void         stop() override;

    [[nodiscard]] const mqtt_status &status()    const { return status_; }
    [[nodiscard]] const mqtt_config &broker_cfg() const { return cfg_; }
    [[nodiscard]] bool               connected() const { return connected_; }

    void mark_network_ready(bool ready) { network_ready_ = ready; }

    blusys_err_t publish(const char *topic, const void *payload, std::size_t len, int qos = -1, bool retain = false);
    blusys_err_t subscribe(const char *topic_filter, int qos = -1);
    blusys_err_t unsubscribe(const char *topic_filter);

    void set_message_handler(mqtt_message_handler_fn handler, void *user_ctx);

    // Internal ingestion points called from device-side service callbacks.
    // These run on the MQTT task thread; they are thread-safe via pending_mtx_.
    void on_mqtt_message(const char *topic, const std::uint8_t *payload, std::size_t len);
    void on_mqtt_state(int state);  // blusys_mqtt_state_t cast in the device backend

private:
    blusys_err_t open_broker();
    void         close_broker();
    void         issue_auto_subscribes();

    void post_ev(mqtt_event ev, const void *payload = nullptr);

    mqtt_config               cfg_;
    mqtt_status               status_{};

    bool                      network_ready_ = false;
    bool                      connected_     = false;

    mutable std::mutex        pending_mtx_{};
    std::vector<mqtt_message> pending_{};
    std::vector<mqtt_message> drained_{};
    std::atomic<bool>         pending_connected_{false};
    std::atomic<bool>         pending_disconnected_{false};
    std::atomic<bool>         pending_error_{false};

    std::uint32_t             next_reconnect_ms_ = 0;
    std::uint32_t             backoff_ms_        = 0;

    // Opaque handle; blusys_mqtt_t* on device, nullptr on host.
    void                     *mqtt_handle_ = nullptr;

    mqtt_message_handler_fn   handler_     = nullptr;
    void                     *handler_ctx_ = nullptr;
};

inline void mqtt_capability::post_ev(mqtt_event ev, const void *payload)
{
    post_integration_event(static_cast<std::uint32_t>(ev), 0, payload);
}

}  // namespace blusys
