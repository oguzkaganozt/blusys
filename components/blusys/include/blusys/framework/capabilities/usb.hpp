#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <cstdint>

// Forward declarations for device-side USB handles.
struct blusys_usb_device;
typedef struct blusys_usb_device blusys_usb_device_t;
struct blusys_usb_host;
typedef struct blusys_usb_host blusys_usb_host_t;

namespace blusys { class runtime; }

namespace blusys {

// ---- shared types (always available) ----

enum class usb_event : std::uint32_t {
    device_ready        = 0x0B00,
    host_ready          = 0x0B01,
    device_connected    = 0x0B10,
    device_disconnected = 0x0B11,
    device_suspended    = 0x0B12,
    device_resumed      = 0x0B13,
    host_attached       = 0x0B20,
    host_detached       = 0x0B21,
    capability_ready    = 0x0BFF,
};

enum class usb_role : std::uint8_t {
    device,
    host,
};

enum class usb_class : std::uint8_t {
    cdc = 1 << 0,
    hid = 1 << 1,
    msc = 1 << 2,
};

struct usb_status : capability_status_base {
    bool role_active      = false;
    bool device_ready     = false;
    bool host_ready       = false;
    bool device_connected = false;
    bool host_attached    = false;
};

// ---- configuration (platform-independent) ----

struct usb_config {
    usb_role     role        = usb_role::device;
    std::uint8_t class_mask  = static_cast<std::uint8_t>(usb_class::cdc);
    const char  *manufacturer = "Blusys";
    const char  *product      = nullptr;
    const char  *serial       = nullptr;
    std::uint16_t vid         = 0x303A;
    std::uint16_t pid         = 0x4001;
};

// ---- capability class ----

class usb_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::usb;

    explicit usb_capability(const usb_config &cfg);
    ~usb_capability() override;

    [[nodiscard]] capability_kind kind() const override { return capability_kind::usb; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const usb_status &status() const { return status_; }

private:
    // Event handlers use int for event type to avoid device-specific enum in header.
    // Device backend casts to the appropriate USB event enum type.
    static void device_event_handler(blusys_usb_device_t *dev, int event, void *user_ctx);
    static void host_event_handler(blusys_usb_host_t *host, int event,
                                   const void *device_info, void *user_ctx);
    void check_capability_ready();
    void post_event(usb_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    struct impl;
    impl *impl_ = nullptr;

    usb_config cfg_;
    usb_status status_{};
};

}  // namespace blusys
