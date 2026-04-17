#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <cstddef>
#include <cstdint>
#include <atomic>

#ifdef ESP_PLATFORM
#include "blusys/services/connectivity/ble_gatt.h"
#include "blusys/services/connectivity/bluetooth.h"
#endif

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
    capability_ready           = 0x03FF,
};

struct bluetooth_status : capability_status_base {
    bool    gap_ready        = false;
    bool    advertising      = false;
    bool    gatt_ready       = false;
    bool    client_connected = false;
    std::uint8_t connected_count = 0;
};

// ---- device implementation ----

#ifdef ESP_PLATFORM

// NimBLE is acquired through services (`blusys_bluetooth_*` / `blusys_ble_gatt_*`),
// which serialize access with HAL `blusys_bt_stack` alongside Wi‑Fi provisioning
// (BLE) and USB HID BLE.

struct bluetooth_config {
    const char *device_name = nullptr;
    bool auto_advertise     = true;

    // GATT server (enabled when services != nullptr)
    const blusys_ble_gatt_svc_def_t *services = nullptr;
    std::size_t svc_count = 0;
    blusys_ble_gatt_conn_cb_t conn_cb     = nullptr;
    void *conn_user_ctx                    = nullptr;
};

class bluetooth_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::bluetooth;

    explicit bluetooth_capability(const bluetooth_config &cfg);

    [[nodiscard]] capability_kind kind() const override { return capability_kind::bluetooth; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const bluetooth_status &status() const { return status_; }

private:
    static constexpr std::uint32_t kPendingNone            = 0;
    static constexpr std::uint32_t kPendingGapReady         = 1 << 0;
    static constexpr std::uint32_t kPendingGattReady        = 1 << 1;
    static constexpr std::uint32_t kPendingClientConnected  = 1 << 2;
    static constexpr std::uint32_t kPendingClientDisconnected = 1 << 3;

    static void gatt_conn_handler(std::uint16_t conn_handle, bool connected,
                                  void *user_ctx);

    void post_event(bluetooth_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }
    void check_capability_ready();

    bluetooth_config cfg_;
    bluetooth_status status_{};
    blusys_bluetooth_t  *bt_   = nullptr;
    blusys_ble_gatt_t   *gatt_ = nullptr;
    std::atomic<std::uint32_t> pending_flags_{kPendingNone};
};

#else  // host stub

struct bluetooth_config {
    const char *device_name = nullptr;
    bool auto_advertise     = true;
};

class bluetooth_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::bluetooth;

    explicit bluetooth_capability(const bluetooth_config &cfg)
        : cfg_(cfg) {}

    [[nodiscard]] capability_kind kind() const override { return capability_kind::bluetooth; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const bluetooth_status &status() const { return status_; }

private:
    void post_event(bluetooth_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    bluetooth_config cfg_;
    bluetooth_status status_{};
    bool first_poll_ = true;
};

#endif  // ESP_PLATFORM

}  // namespace blusys
