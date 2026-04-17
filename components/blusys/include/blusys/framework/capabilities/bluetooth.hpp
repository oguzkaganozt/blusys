#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

// Forward declarations for device-side BLE handles and GATT config types.
// The concrete struct/typedef definitions live in the device service headers.
struct blusys_bluetooth;
typedef struct blusys_bluetooth blusys_bluetooth_t;
struct blusys_ble_gatt;
typedef struct blusys_ble_gatt blusys_ble_gatt_t;
struct blusys_ble_gatt_svc_def_t;
using blusys_ble_gatt_conn_cb_t = void (*)(std::uint16_t conn_handle,
                                           bool connected,
                                           void *user_ctx);

namespace blusys { class runtime; }

namespace blusys {

// ---- shared types (always available) ----

enum class bluetooth_event : std::uint32_t {
    gap_ready              = 0x0300,
    advertising_started    = 0x0301,
    advertising_stopped    = 0x0302,
    scan_result            = 0x0303,
    gatt_ready             = 0x0310,
    client_connected       = 0x0311,
    client_disconnected    = 0x0312,
    characteristic_written = 0x0313,
    capability_ready       = 0x03FF,
};

struct bluetooth_status : capability_status_base {
    bool         gap_ready        = false;
    bool         advertising      = false;
    bool         gatt_ready       = false;
    bool         client_connected = false;
    std::uint8_t connected_count  = 0;
};

// ---- configuration ----

struct bluetooth_config {
    const char *device_name = nullptr;
    bool auto_advertise     = true;

    // GATT server (enabled when services != nullptr).
    // On host these fields are unused; set them only in device entry code.
    const blusys_ble_gatt_svc_def_t *services   = nullptr;
    std::size_t                       svc_count  = 0;
    blusys_ble_gatt_conn_cb_t         conn_cb    = nullptr;
    void                             *conn_user_ctx = nullptr;
};

// ---- capability class ----

class bluetooth_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::bluetooth;

    explicit bluetooth_capability(const bluetooth_config &cfg);
    ~bluetooth_capability() override;

    [[nodiscard]] capability_kind kind() const override { return capability_kind::bluetooth; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const bluetooth_status &status() const { return status_; }

private:
    static void gatt_conn_handler(std::uint16_t conn_handle, bool connected,
                                  void *user_ctx);
    void check_capability_ready();
    void post_event(bluetooth_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    struct impl;
    impl *impl_ = nullptr;

    bluetooth_config cfg_;
    bluetooth_status status_{};
};

}  // namespace blusys
