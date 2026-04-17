#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <cstddef>
#include <cstdint>

// Forward declarations for device-side service handles.
struct blusys_wifi;          typedef struct blusys_wifi          blusys_wifi_t;
struct blusys_sntp;          typedef struct blusys_sntp          blusys_sntp_t;
struct blusys_mdns;          typedef struct blusys_mdns          blusys_mdns_t;
struct blusys_local_ctrl;    typedef struct blusys_local_ctrl    blusys_local_ctrl_t;
struct blusys_wifi_prov;     typedef struct blusys_wifi_prov     blusys_wifi_prov_t;

// Forward declarations for config types that use service-layer structs.
struct blusys_local_ctrl_action_t;
using blusys_local_ctrl_status_cb_t =
    blusys_err_t (*)(char *json_buf, std::size_t json_buf_size, void *user_ctx);

namespace blusys { class runtime; }

namespace blusys {

// ---- shared types (always available) ----

enum class connectivity_event : std::uint32_t {
    wifi_started              = 0x0100,
    wifi_connecting           = 0x0101,
    wifi_connected            = 0x0102,
    wifi_got_ip               = 0x0103,
    wifi_disconnected         = 0x0104,
    wifi_reconnecting         = 0x0105,
    time_synced               = 0x0110,
    time_sync_failed          = 0x0111,
    mdns_ready                = 0x0120,
    local_ctrl_ready          = 0x0130,
    prov_started              = 0x0140,
    prov_credentials_received = 0x0141,
    prov_success              = 0x0142,
    prov_failed               = 0x0143,
    prov_already_done         = 0x0144,
    prov_reset_complete       = 0x0145,
    provisioning_ready        = 0x014F,
    capability_ready          = 0x01FF,
};

// IP address info — uniform struct on both host and device.
struct connectivity_ip_info {
    char ip[16]      = "192.168.1.100";
    char netmask[16] = "255.255.255.0";
    char gateway[16] = "192.168.1.1";
};

struct connectivity_provisioning_status {
    bool is_provisioned       = false;
    bool service_running      = false;
    bool credentials_received = false;
    bool provision_success    = false;
    bool provision_failed     = false;
    bool capability_ready     = false;
    char qr_payload[256]      = {};
};

struct connectivity_status : capability_status_base {
    bool wifi_connected     = false;
    bool wifi_connecting    = false;
    bool wifi_reconnecting  = false;
    bool has_ip             = false;
    bool time_synced        = false;
    bool mdns_running       = false;
    bool local_ctrl_running = false;
    connectivity_ip_info ip_info{};
    std::int8_t wifi_rssi   = 0;
    connectivity_provisioning_status provisioning{};
};

// ---- configuration ----

struct connectivity_config {
    // Wi-Fi (enabled when ssid != nullptr)
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
    const char                       *local_ctrl_device_name  = nullptr;
    std::uint16_t                     local_ctrl_port         = 80;
    const blusys_local_ctrl_action_t *local_ctrl_actions      = nullptr;
    std::size_t                       local_ctrl_action_count = 0;
    blusys_local_ctrl_status_cb_t     local_ctrl_status_cb    = nullptr;
    void                             *local_ctrl_user_ctx     = nullptr;

    // Provisioning (enabled when service_name != nullptr).
    // transport: 0 = BLE, 1 = SoftAP (matches BLUSYS_WIFI_PROV_TRANSPORT_BLE/SOFTAP).
    int         prov_transport          = 0;
    const char *prov_service_name       = nullptr;
    const char *prov_pop                = nullptr;
    const char *prov_service_key        = nullptr;
    bool        prov_auto_start         = true;
    bool        prov_skip_if_provisioned = true;
};

// ---- capability class ----

class connectivity_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::connectivity;

    explicit connectivity_capability(const connectivity_config &cfg);
    ~connectivity_capability() override;

    [[nodiscard]] capability_kind kind() const override { return capability_kind::connectivity; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const connectivity_status &status() const { return status_; }

    blusys_err_t request_reconnect();

private:
    // Device-side event handlers; int used for event type to avoid device enums in header.
    static void wifi_event_handler(blusys_wifi_t *wifi, int event,
                                   const void *info, void *user_ctx);
    static void prov_event_handler(int event, const void *creds, void *user_ctx);
    void start_dependent_services(std::uint32_t now_ms);
    void check_capability_ready();
    void post_event(connectivity_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    struct impl;
    impl *impl_ = nullptr;

    connectivity_config cfg_;
    connectivity_status status_{};
    bool first_poll_ = true;
};

}  // namespace blusys
