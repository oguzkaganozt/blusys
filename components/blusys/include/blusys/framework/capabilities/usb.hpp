#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <atomic>
#include <cstdint>

#ifdef ESP_PLATFORM
#include "blusys/hal/target.h"
#include "blusys/hal/usb_device.h"
#include "blusys/hal/usb_host.h"
#endif

namespace blusys { class runtime; }

namespace blusys {

enum class usb_event : std::uint32_t {
    device_ready       = 0x0B00,
    host_ready         = 0x0B01,
    device_connected   = 0x0B10,
    device_disconnected = 0x0B11,
    device_suspended   = 0x0B12,
    device_resumed     = 0x0B13,
    host_attached      = 0x0B20,
    host_detached      = 0x0B21,
    capability_ready   = 0x0BFF,
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
    bool role_active = false;
    bool device_ready = false;
    bool host_ready = false;
    bool device_connected = false;
    bool host_attached = false;
};

// Exactly one of host or device mode per product; `usb_capability` opens only
// the selected role (mutually exclusive at runtime).
struct usb_config {
    usb_role role = usb_role::device;
    std::uint8_t class_mask = static_cast<std::uint8_t>(usb_class::cdc);
    const char *manufacturer = "Blusys";
    const char *product = nullptr;
    const char *serial = nullptr;
    std::uint16_t vid = 0x303A;
    std::uint16_t pid = 0x4001;
};

#ifdef ESP_PLATFORM

class usb_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::usb;

    explicit usb_capability(const usb_config &cfg);

    [[nodiscard]] capability_kind kind() const override { return capability_kind::usb; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const usb_status &status() const { return status_; }

private:
    static constexpr std::uint32_t kPendingNone             = 0;
    static constexpr std::uint32_t kPendingDeviceConnected  = 1 << 0;
    static constexpr std::uint32_t kPendingDeviceDisconnected = 1 << 1;
    static constexpr std::uint32_t kPendingDeviceSuspended  = 1 << 2;
    static constexpr std::uint32_t kPendingDeviceResumed    = 1 << 3;
    static constexpr std::uint32_t kPendingHostAttached     = 1 << 4;
    static constexpr std::uint32_t kPendingHostDetached     = 1 << 5;

    static void device_event_handler(blusys_usb_device_t *dev,
                                     blusys_usb_device_event_t event,
                                     void *user_ctx);
    static void host_event_handler(blusys_usb_host_t *host,
                                   blusys_usb_host_event_t event,
                                   const blusys_usb_host_device_info_t *device,
                                   void *user_ctx);

    void post_event(usb_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }
    void check_capability_ready();
    [[nodiscard]] blusys_usb_device_class_t selected_device_class() const;

    usb_config cfg_;
    usb_status status_{};
    blusys_usb_device_t *device_ = nullptr;
    blusys_usb_host_t *host_ = nullptr;
    std::atomic<std::uint32_t> pending_flags_{kPendingNone};
};

#else

class usb_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::usb;

    explicit usb_capability(const usb_config &cfg)
        : cfg_(cfg) {}

    [[nodiscard]] capability_kind kind() const override { return capability_kind::usb; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const usb_status &status() const { return status_; }

private:
    void post_event(usb_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    usb_config cfg_;
    usb_status status_{};
    bool first_poll_ = true;
};

#endif

}  // namespace blusys
