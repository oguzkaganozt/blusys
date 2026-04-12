#pragma once

#include "blusys/app/capability.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace blusys::framework {
class runtime;
}

namespace blusys::app {

// Well-known integration event IDs posted by mqtt_host_capability (host / SDL only).
// Reserved range — keep aligned with capability.hpp.
enum class mqtt_host_event : std::uint32_t {
    connected        = 0x0800,
    disconnected     = 0x0801,
    message_received = 0x0802,
    publish_failed   = 0x0803,
    error            = 0x080E,
    capability_ready = 0x08FF,
};

struct mqtt_host_status {
    bool          connected         = false;
    bool          capability_ready  = false;
    std::uint32_t messages_rx       = 0;
    std::uint32_t publishes_tx      = 0;
    char          last_error[96]    = {};
    char          last_topic[128]   = {};
    char          last_payload[256] = {};
};

// Configuration for the SDL/host MQTT client (libmosquitto). Not used on ESP-IDF.
struct mqtt_host_config {
    const char *broker_url          = nullptr;
    const char *client_id           = nullptr;
    const char *subscribe_topics[8] = {};
    int         keepalive_sec       = 60;
    int         qos                 = 1;
};

class mqtt_host_capability final : public capability_base {
public:
    explicit mqtt_host_capability(const mqtt_host_config &cfg);

    mqtt_host_capability(const mqtt_host_capability &)            = delete;
    mqtt_host_capability &operator=(const mqtt_host_capability &) = delete;

    [[nodiscard]] capability_kind kind() const override { return capability_kind::mqtt_host; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void         poll(std::uint32_t now_ms) override;
    void         stop() override;

    [[nodiscard]] const mqtt_host_status &status() const { return status_; }

    [[nodiscard]] const mqtt_host_config &broker_config() const { return cfg_; }

    // Call from the app thread only (e.g. inside update()).
    blusys_err_t publish(const char *topic, const void *payload, std::size_t len, int qos = -1);

    // libmosquitto worker thread — enqueue only; poll() posts to runtime on the app thread.
    void notify_connected();
    void notify_disconnected();
    void notify_message(const char *topic, std::size_t topic_len, const void *payload,
                        std::size_t payload_len);

    void notify_error(const char *msg);

private:
    struct Pending {
        mqtt_host_event ev = mqtt_host_event::error;
        bool            has_message = false;
        char            topic[128]  = {};
        char            payload[256] = {};
        std::size_t     payload_len = 0;
    };

    void flush_pending();
    void post_ev(mqtt_host_event ev);

    mqtt_host_config            cfg_{};
    mqtt_host_status            status_{};
    blusys::framework::runtime *rt_ = nullptr;
    void                       *mosq_         = nullptr;
    bool                        started_lib_ = false;

    mutable std::mutex   pending_mtx_{};
    std::vector<Pending> pending_{};
};

}  // namespace blusys::app
