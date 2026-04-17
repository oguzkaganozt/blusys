#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <cstdint>

// Forward declaration for the device-side BLE HID handle.
struct blusys_ble_hid_device;
typedef struct blusys_ble_hid_device blusys_ble_hid_device_t;

namespace blusys { class runtime; }

namespace blusys {

// ---- shared types (always available) ----

enum class ble_hid_device_event : std::uint32_t {
    advertising_started = 0x0C01,
    client_connected    = 0x0C02,
    client_disconnected = 0x0C03,
    capability_ready    = 0x0CFF,
};

struct ble_hid_device_status : capability_status_base {
    bool advertising      = false;
    bool client_connected = false;
};

// ---- configuration (platform-independent) ----

struct ble_hid_device_config {
    const char   *device_name         = nullptr;
    std::uint16_t vendor_id           = 0;
    std::uint16_t product_id          = 0;
    std::uint16_t version             = 0;
    std::uint8_t  initial_battery_pct = 100;
};

// ---- capability class ----

class ble_hid_device_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::ble_hid_device;

    explicit ble_hid_device_capability(const ble_hid_device_config &cfg);
    ~ble_hid_device_capability() override;

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
    [[nodiscard]] bool is_connected() const;
    [[nodiscard]] bool is_ready() const;

private:
    static void conn_handler(bool connected, void *user_ctx);
    void check_capability_ready();
    void post_event(ble_hid_device_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    struct impl;
    impl *impl_ = nullptr;

    ble_hid_device_config cfg_;
    ble_hid_device_status status_{};
};

}  // namespace blusys
