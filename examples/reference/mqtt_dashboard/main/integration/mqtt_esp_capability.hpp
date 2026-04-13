#pragma once

#ifdef ESP_PLATFORM

#include "blusys/app/capability.hpp"
#include "blusys/app/capabilities/mqtt_host.hpp"

#include "blusys/error.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct blusys_mqtt;

namespace blusys::framework {
class runtime;
}

namespace mqtt_dashboard::system {

struct mqtt_esp_config {
    const char *broker_url;
    int         mqtt_connect_timeout_ms;
    /** e.g. "blusys/demo/#" */
    const char *subscribe_pattern;
    int         qos;
};

/** Device MQTT using blusys_mqtt (ESP-IDF); posts the same integration IDs as mqtt_host (0x08xx). */
class mqtt_esp_capability final : public blusys::app::capability_base {
public:
    explicit mqtt_esp_capability(const mqtt_esp_config &cfg);

    [[nodiscard]] blusys::app::capability_kind kind() const override
    {
        return blusys::app::capability_kind::custom;
    }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void         poll(std::uint32_t now_ms) override;
    void         stop() override;

    [[nodiscard]] const blusys::app::mqtt_host_status &status() const { return status_; }

    blusys_err_t publish(const char *topic, const void *payload, std::size_t len, int qos);

private:
    enum class phase : std::uint8_t {
        wait_sta_ip,
        mqtt_connecting,
        running,
        failed,
    };

    static void message_trampoline(const char *topic, const std::uint8_t *payload, std::size_t len,
                                   void *user_ctx);

    void enqueue_incoming(const char *topic, const std::uint8_t *payload, std::size_t len);
    void flush_pending_incoming();
    bool sta_has_ip() const;
    void try_start_mqtt_after_ip();

    mqtt_esp_config                 cfg_{};
    blusys::app::mqtt_host_status   status_{};
    blusys_mqtt                    *mqtt_ = nullptr;
    phase                           phase_ = phase::wait_sta_ip;
    bool                            connect_attempted_ = false;

    struct pending_in {
        std::string topic;
        std::string payload;
    };
    std::mutex                 incoming_mtx_{};
    std::vector<pending_in>    incoming_{};
};

/** Registered instance for app_logic on ESP (nullptr on host). */
[[nodiscard]] mqtt_esp_capability *mqtt_esp();

}  // namespace mqtt_dashboard::system

#endif  // ESP_PLATFORM
