#pragma once

// Device-side MQTT capability. Wraps the `blusys_mqtt` C service and
// lifts topic traffic into the canonical capability event stream
// (raw IDs 0x08xx band, same as `mqtt_host_capability`).
//
// Composition model — products should NOT embed an MQTT client in
// their own code. Instead:
//
//   blusys::mqtt_capability mqtt{mqtt_config{...}};
//   blusys::capability_list caps{&connectivity, &mqtt, ...};
//
//   // After Wi-Fi + SNTP come up (observe in reducer):
//   mqtt.mark_network_ready(true);
//
//   // Send:
//   mqtt.publish("topic", payload, len, QoS, retain);
//
//   // Receive: reducer sees `capability_event_tag::mqtt_message_received`
//   // events with an `mqtt_message *` payload, OR register an
//   // on-app-thread direct handler with `set_message_handler`.

#include "blusys/framework/capabilities/capability.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#ifdef ESP_PLATFORM
#include "blusys/services/protocol/mqtt.h"
#endif

namespace blusys { class runtime; }

namespace blusys {

// Raw integration event IDs posted by `mqtt_capability`. Match
// `mqtt_host_event` so the shared `capability_event_map` delivers both
// classes into the same `capability_event_tag::mqtt_*` symbols.
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
// it until the NEXT poll(). Reducer reads synchronously.
//
// QoS and retain flags are NOT surfaced by the underlying blusys_mqtt
// service's message_cb, so they're omitted here. Add them to the service
// first if a product ever needs them.
struct mqtt_message {
    char         topic[160]   = {};
    char         payload[512] = {};   // nul-terminated copy
    std::size_t  payload_len  = 0;
};

struct mqtt_status {
    bool          connected         = false;
    bool          capability_ready  = false;
    std::uint32_t messages_rx       = 0;
    std::uint32_t publishes_tx      = 0;
    char          last_error[96]    = {};
};

// Product-set direct handler — invoked on the app thread from
// `poll()` as each message is drained. Receives a pointer into the
// drained buffer; the pointer is valid for the duration of the call.
using mqtt_message_handler_fn = void (*)(const mqtt_message &msg, void *user_ctx);

struct mqtt_config {
    const char *broker_url       = nullptr;   // "mqtt://host:1883" or "mqtts://host:8883"
    const char *client_id        = nullptr;   // NULL = auto
    const char *username         = nullptr;
    const char *password         = nullptr;
    const char *server_cert_pem = nullptr;    // PEM for MQTTS; NULL skips verify
    int         connect_timeout_ms = 15000;

    // Topics subscribed automatically on every (re)connect. NULL entries
    // terminate the list.
    const char *auto_subscribe_topics[8] = {};
    int         default_qos      = 1;

    // Reconnect backoff.
    std::uint32_t reconnect_min_ms = 2000;
    std::uint32_t reconnect_max_ms = 60000;

    // Optional MQTT Last-Will-and-Testament. Broker publishes the will
    // payload if the device disconnects ungracefully. Set `will_topic`
    // to a non-null pointer to enable; leave NULL to disable.
    const char   *will_topic       = nullptr;
    const char   *will_payload     = nullptr;   // may be NULL for zero-length
    std::size_t   will_payload_len = 0;          // 0 → strlen(will_payload)
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

    // Product gates broker dial on network state; framework can't
    // assume Wi-Fi+SNTP — those sit behind `connectivity_capability`
    // which posts `wifi_got_ip` / `time_synced` events the reducer
    // observes. Mirror that into here from `on_tick` or the reducer.
    // Plain bool: caller and consumer both run on the app thread.
    void mark_network_ready(bool ready) { network_ready_ = ready; }

    // App-thread API. Returns BLUSYS_ERR_INVALID_STATE if not connected.
    blusys_err_t publish(const char *topic,
                         const void *payload, std::size_t len,
                         int qos = -1, bool retain = false);
    blusys_err_t subscribe(const char *topic_filter, int qos = -1);
    blusys_err_t unsubscribe(const char *topic_filter);

    // Direct, app-thread message delivery. Per-message delivery is still
    // emitted as an integration event so the reducer can observe it too.
    void set_message_handler(mqtt_message_handler_fn handler, void *user_ctx);

private:
#ifdef ESP_PLATFORM
    static void mqtt_cb(const char *topic, const std::uint8_t *payload,
                        std::size_t payload_len, void *user_ctx);
    static void state_cb(blusys_mqtt_state_t state, void *user_ctx);
#endif

    blusys_err_t open_broker();
    void         close_broker();
    void         issue_auto_subscribes();

    void post_ev(mqtt_event ev, const void *payload = nullptr)
    {
        post_integration_event(static_cast<std::uint32_t>(ev), 0, payload);
    }

    mqtt_config               cfg_;
    mqtt_status               status_{};

    // App-thread flags. network_ready_ is written by the product from
    // on_tick; connected_ is written only by poll() after it drains
    // pending_* transitions set from the MQTT task.
    bool                      network_ready_ = false;
    bool                      connected_     = false;

    // Thread bridges — written by the MQTT task, drained by poll().
    mutable std::mutex        pending_mtx_{};
    std::vector<mqtt_message> pending_{};
    std::vector<mqtt_message> drained_{};      // persists so reducer can read payload*
    std::atomic<bool>         pending_connected_{false};
    std::atomic<bool>         pending_disconnected_{false};
    std::atomic<bool>         pending_error_{false};

    // Connect / reconnect bookkeeping (app thread).
    std::uint32_t             next_reconnect_ms_ = 0;
    std::uint32_t             backoff_ms_        = 0;

#ifdef ESP_PLATFORM
    blusys_mqtt_t            *mqtt_handle_ = nullptr;
#else
    void                     *mqtt_handle_ = nullptr;
#endif

    mqtt_message_handler_fn   handler_        = nullptr;
    void                     *handler_ctx_    = nullptr;
};

}  // namespace blusys
