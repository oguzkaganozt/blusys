#pragma once

#include "blusys/app/capability.hpp"

#include <cstddef>
#include <cstdint>
#include <atomic>

#ifdef ESP_PLATFORM
#include "blusys/connectivity/mdns.h"
#include "blusys/connectivity/wifi.h"
#include "blusys/system/local_ctrl.h"
#include "blusys/system/sntp.h"
#endif

namespace blusys::framework { class runtime; }

namespace blusys::app {

// ---- shared types (always available) ----

// Well-known integration event IDs posted by the connectivity capability.
// Apps match on these in map_event() to convert into app-specific actions.
enum class connectivity_event : std::uint32_t {
    wifi_started       = 0x0100,
    wifi_connecting     = 0x0101,
    wifi_connected      = 0x0102,
    wifi_got_ip         = 0x0103,
    wifi_disconnected   = 0x0104,
    wifi_reconnecting   = 0x0105,
    time_synced         = 0x0110,
    time_sync_failed    = 0x0111,
    mdns_ready          = 0x0120,
    local_ctrl_ready    = 0x0130,
    capability_ready        = 0x01FF,
};

// IP info — mirrors the C struct on device, standalone on host.
#ifdef ESP_PLATFORM
using connectivity_ip_info = blusys_wifi_ip_info_t;
#else
struct connectivity_ip_info {
    char ip[16]      = "192.168.1.100";
    char netmask[16] = "255.255.255.0";
    char gateway[16] = "192.168.1.1";
};
#endif

// Current connectivity state, queryable via ctx.connectivity().
struct connectivity_status {
    bool wifi_connected     = false;
    bool wifi_connecting    = false;  // initial association or reconnect in progress
    bool wifi_reconnecting  = false;  // driver reported reconnecting after loss
    bool has_ip             = false;
    bool time_synced        = false;
    bool mdns_running       = false;
    bool local_ctrl_running = false;
    bool capability_ready       = false;
    connectivity_ip_info ip_info{};
    std::int8_t wifi_rssi   = 0;
};

// ---- device implementation ----

#ifdef ESP_PLATFORM

// Declarative connectivity configuration.
// Each subsystem is optional — enabled when its key field is non-null.
struct connectivity_config {
    // Wi-Fi (required)
    const char *wifi_ssid          = nullptr;
    const char *wifi_password      = nullptr;
    bool        auto_reconnect     = true;
    int         reconnect_delay_ms = 1000;
    int         max_retries        = -1;
    int         connect_timeout_ms = 10000;

    // SNTP (enabled when server != nullptr)
    const char *sntp_server        = nullptr;
    const char *sntp_server2       = nullptr;
    bool        sntp_smooth_sync   = false;
    int         sntp_timeout_ms    = 10000;

    // mDNS (enabled when hostname != nullptr)
    const char *mdns_hostname      = nullptr;
    const char *mdns_instance_name = nullptr;

    // Local control (enabled when device_name != nullptr)
    const char *local_ctrl_device_name           = nullptr;
    std::uint16_t local_ctrl_port                = 80;
    const blusys_local_ctrl_action_t *local_ctrl_actions = nullptr;
    std::size_t local_ctrl_action_count          = 0;
    blusys_local_ctrl_status_cb_t local_ctrl_status_cb = nullptr;
    void *local_ctrl_user_ctx                    = nullptr;
};

class connectivity_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::connectivity;

    explicit connectivity_capability(const connectivity_config &cfg);

    [[nodiscard]] capability_kind kind() const override { return capability_kind::connectivity; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const connectivity_status &status() const { return status_; }

    // Explicitly trigger a reconnect attempt. Products call this from
    // system/ in response to a reducer action.
    blusys_err_t request_reconnect();

private:
    static constexpr std::uint32_t kPendingNone         = 0;
    static constexpr std::uint32_t kPendingConnected    = 1 << 0;
    static constexpr std::uint32_t kPendingGotIp        = 1 << 1;
    static constexpr std::uint32_t kPendingDisconnected = 1 << 2;
    static constexpr std::uint32_t kPendingReconnecting = 1 << 3;
    static constexpr std::uint32_t kPendingManualConnecting = 1 << 4;
    static constexpr std::uint32_t kPendingDriverStarted    = 1 << 5;
    static constexpr std::uint32_t kPendingDriverConnecting = 1 << 6;

    static void wifi_event_handler(blusys_wifi_t *wifi,
                                   blusys_wifi_event_t event,
                                   const blusys_wifi_event_info_t *info,
                                   void *user_ctx);

    void start_dependent_services(std::uint32_t now_ms);
    void post_event(connectivity_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }
    void check_capability_ready();

    connectivity_config   cfg_;
    connectivity_status   status_{};

    blusys_wifi_t         *wifi_  = nullptr;
    blusys_sntp_t         *sntp_  = nullptr;
    blusys_mdns_t         *mdns_  = nullptr;
    blusys_local_ctrl_t   *ctrl_  = nullptr;

    std::atomic<std::uint32_t> pending_flags_{kPendingNone};
    bool dependent_services_started_ = false;
    std::uint32_t sntp_sync_started_ms_ = 0;
    bool sntp_timeout_reported_ = false;
};

#else  // host stub

// Host-side config — same shape, but fields are only used to decide
// which simulated events to post.
struct connectivity_config {
    const char *wifi_ssid          = nullptr;
    const char *wifi_password      = nullptr;
    bool        auto_reconnect     = true;
    int         reconnect_delay_ms = 1000;
    int         max_retries        = -1;
    int         connect_timeout_ms = 10000;

    const char *sntp_server        = nullptr;
    const char *sntp_server2       = nullptr;
    bool        sntp_smooth_sync   = false;
    int         sntp_timeout_ms    = 10000;

    const char *mdns_hostname      = nullptr;
    const char *mdns_instance_name = nullptr;

    const char *local_ctrl_device_name = nullptr;
    std::uint16_t local_ctrl_port      = 80;
};

class connectivity_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::connectivity;

    explicit connectivity_capability(const connectivity_config &cfg)
        : cfg_(cfg) {}

    [[nodiscard]] capability_kind kind() const override { return capability_kind::connectivity; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const connectivity_status &status() const { return status_; }

    blusys_err_t request_reconnect();

private:
    void post_event(connectivity_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    connectivity_config   cfg_;
    connectivity_status   status_{};
    bool first_poll_ = true;
};

#endif  // ESP_PLATFORM

}  // namespace blusys::app
