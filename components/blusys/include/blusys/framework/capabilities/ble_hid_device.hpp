#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <atomic>
#include <cstdint>

#ifdef ESP_PLATFORM
#include "blusys/services/connectivity/ble_hid_device.h"
#endif

namespace blusys { class runtime; }

namespace blusys {

enum class ble_hid_device_event : std::uint32_t {
    advertising_started = 0x0C01,
    client_connected    = 0x0C02,
    client_disconnected = 0x0C03,
    capability_ready    = 0x0CFF,
};

struct ble_hid_device_status {
    bool    advertising      = false;
    bool    client_connected = false;
    bool    capability_ready = false;
};

struct ble_hid_device_config {
    const char   *device_name         = nullptr;
    std::uint16_t vendor_id           = 0;
    std::uint16_t product_id          = 0;
    std::uint16_t version             = 0;
    std::uint8_t  initial_battery_pct = 100;
};

#ifdef ESP_PLATFORM

class ble_hid_device_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::ble_hid_device;

    explicit ble_hid_device_capability(const ble_hid_device_config &cfg);

    [[nodiscard]] capability_kind kind() const override
    {
        return capability_kind::ble_hid_device;
    }

    blusys_err_t start(blusys::runtime &rt) override;
    void         poll(std::uint32_t now_ms) override;
    void         stop() override;

    [[nodiscard]] const ble_hid_device_status &status() const { return status_; }

    /// Edge-triggered consumer-control update. No-ops when no client is
    /// subscribed. Safe to call from the reducer or other app threads.
    blusys_err_t send_consumer(std::uint16_t usage, bool pressed);

    /// Update the advertised battery level (0..100).
    blusys_err_t set_battery(std::uint8_t percent);

    [[nodiscard]] bool is_connected() const;

    /// True once the paired host has also subscribed to the Input Report
    /// characteristic. Only then will notifications actually reach the OS.
    [[nodiscard]] bool is_ready() const;

private:
    static constexpr std::uint32_t kPendingNone                 = 0;
    static constexpr std::uint32_t kPendingAdvertisingStarted   = 1u << 0;
    static constexpr std::uint32_t kPendingClientConnected      = 1u << 1;
    static constexpr std::uint32_t kPendingClientDisconnected   = 1u << 2;

    static void conn_handler(bool connected, void *user_ctx);

    void post_event(ble_hid_device_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }
    void check_capability_ready();

    ble_hid_device_config     cfg_;
    ble_hid_device_status     status_{};
    blusys_ble_hid_device_t  *hid_ = nullptr;
    std::atomic<std::uint32_t> pending_flags_{kPendingNone};
};

#else  // host stub

class ble_hid_device_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::ble_hid_device;

    explicit ble_hid_device_capability(const ble_hid_device_config &cfg)
        : cfg_(cfg) {}

    [[nodiscard]] capability_kind kind() const override
    {
        return capability_kind::ble_hid_device;
    }

    blusys_err_t start(blusys::runtime &rt) override;
    void         poll(std::uint32_t now_ms) override;
    void         stop() override;

    [[nodiscard]] const ble_hid_device_status &status() const { return status_; }

    blusys_err_t send_consumer(std::uint16_t usage, bool pressed);
    blusys_err_t set_battery(std::uint8_t percent);
    [[nodiscard]] bool is_connected() const { return status_.client_connected; }
    [[nodiscard]] bool is_ready() const { return status_.client_connected; }

private:
    void post_event(ble_hid_device_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    ble_hid_device_config cfg_;
    ble_hid_device_status status_{};
    std::uint8_t          consumer_state_ = 0;
    std::uint8_t          battery_level_  = 100;
    std::uint32_t         first_ready_ms_ = 0;
    bool                  first_poll_     = true;
    bool                  emitted_ready_  = false;
};

#endif  // ESP_PLATFORM

}  // namespace blusys
