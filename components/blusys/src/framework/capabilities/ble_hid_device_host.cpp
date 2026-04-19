#include "blusys/framework/capabilities/ble_hid_device.hpp"
#include "blusys/framework/events/event_queue.hpp"
#include "blusys/hal/log.h"

#include <cstdio>
#include <cstdint>

static const char *TAG = "blusys_ble_hid";

namespace blusys {

struct ble_hid_device_capability::impl {
    bool          first_poll    = true;
    bool          emitted_ready = false;
    std::uint32_t first_ready_ms = 0;
    std::uint8_t  consumer_state = 0;
    std::uint8_t  battery_level  = 100;
};

ble_hid_device_capability::ble_hid_device_capability(const ble_hid_device_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

ble_hid_device_capability::~ble_hid_device_capability()
{
    delete impl_;
}

blusys_err_t ble_hid_device_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    impl_->first_poll    = true;
    impl_->emitted_ready = false;
    impl_->first_ready_ms = 0;
    impl_->battery_level = cfg_.initial_battery_pct > 100 ? 100 : cfg_.initial_battery_pct;
    if (impl_->battery_level == 0 && cfg_.initial_battery_pct == 0) {
        impl_->battery_level = 100;
    }

    status_.advertising = true;
    BLUSYS_LOGI(TAG, "[ble_hid_device] open '%s' (host stub)",
                cfg_.device_name ? cfg_.device_name : "(null)");
    return BLUSYS_OK;
}

void ble_hid_device_capability::poll(std::uint32_t now_ms)
{
    if (impl_->first_poll) {
        impl_->first_poll = false;
        impl_->first_ready_ms = now_ms;
        post_event(ble_hid_device_event::advertising_started);
        return;
    }

    if (!impl_->emitted_ready && now_ms >= impl_->first_ready_ms + 1000u) {
        impl_->emitted_ready = true;
        status_.capability_ready = true;
        status_.client_connected = true;
        post_event(ble_hid_device_event::client_connected);
        post_event(ble_hid_device_event::capability_ready);
        BLUSYS_LOGI(TAG, "[ble_hid_device] simulated client connected");
    }
}

void ble_hid_device_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
    BLUSYS_LOGI(TAG, "[ble_hid_device] close (host stub)");
}

void ble_hid_device_capability::conn_handler(bool /*connected*/, void * /*user_ctx*/)
{
}

void ble_hid_device_capability::check_capability_ready()
{
}

blusys_err_t ble_hid_device_capability::send_consumer(std::uint16_t usage, bool pressed)
{
    if (!status_.client_connected) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    int bit = -1;
    switch (usage) {
    case 0x00E9: bit = 0; break;
    case 0x00EA: bit = 1; break;
    case 0x00E2: bit = 2; break;
    case 0x00CD: bit = 3; break;
    case 0x00B5: bit = 4; break;
    case 0x00B6: bit = 5; break;
    case 0x006F: bit = 6; break;
    case 0x0070: bit = 7; break;
    default: return BLUSYS_ERR_INVALID_ARG;
    }
    if (pressed) {
        impl_->consumer_state |= static_cast<std::uint8_t>(1u << bit);
    } else {
        impl_->consumer_state &= static_cast<std::uint8_t>(~(1u << bit));
    }
    std::printf("[ble_hid_device] consumer usage=0x%04x pressed=%d state=0x%02x\n",
                usage, pressed ? 1 : 0, impl_->consumer_state);
    return BLUSYS_OK;
}

blusys_err_t ble_hid_device_capability::set_battery(std::uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }
    impl_->battery_level = percent;
    std::printf("[ble_hid_device] battery=%u%%\n",
                static_cast<unsigned>(impl_->battery_level));
    return BLUSYS_OK;
}

bool ble_hid_device_capability::is_connected() const
{
    return status_.client_connected;
}

bool ble_hid_device_capability::is_ready() const
{
    return status_.capability_ready;
}

}  // namespace blusys
