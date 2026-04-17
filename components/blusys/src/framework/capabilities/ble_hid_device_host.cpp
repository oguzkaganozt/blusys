#include "blusys/framework/capabilities/ble_hid_device.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

#include <cstdio>

static const char *TAG = "blusys_ble_hid";

namespace blusys {

blusys_err_t ble_hid_device_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    first_poll_     = true;
    emitted_ready_  = false;
    first_ready_ms_ = 0;
    battery_level_  = cfg_.initial_battery_pct > 100 ? 100 : cfg_.initial_battery_pct;
    if (battery_level_ == 0 && cfg_.initial_battery_pct == 0) {
        battery_level_ = 100;
    }

    status_.advertising = true;
    BLUSYS_LOGI(TAG, "[ble_hid_device] open '%s' (host stub)",
                cfg_.device_name ? cfg_.device_name : "(null)");
    return BLUSYS_OK;
}

void ble_hid_device_capability::poll(std::uint32_t now_ms)
{
    if (first_poll_) {
        first_poll_ = false;
        first_ready_ms_ = now_ms;
        post_event(ble_hid_device_event::advertising_started);
        return;
    }

    if (!emitted_ready_ && now_ms >= first_ready_ms_ + 1000u) {
        emitted_ready_ = true;
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

blusys_err_t ble_hid_device_capability::send_consumer(std::uint16_t usage, bool pressed)
{
    if (!status_.client_connected) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    int bit = -1;
    switch (usage) {
    case 0x00E9: bit = 0; break;  /* Vol+      */
    case 0x00EA: bit = 1; break;  /* Vol-      */
    case 0x00E2: bit = 2; break;  /* Mute      */
    case 0x00CD: bit = 3; break;  /* Play/P    */
    case 0x00B5: bit = 4; break;  /* Next      */
    case 0x00B6: bit = 5; break;  /* Prev      */
    case 0x006F: bit = 6; break;  /* Bright+   */
    case 0x0070: bit = 7; break;  /* Bright-   */
    default: return BLUSYS_ERR_INVALID_ARG;
    }
    if (pressed) {
        consumer_state_ |= static_cast<std::uint8_t>(1u << bit);
    } else {
        consumer_state_ &= static_cast<std::uint8_t>(~(1u << bit));
    }
    std::printf("[ble_hid_device] consumer usage=0x%04x pressed=%d state=0x%02x\n",
                usage, pressed ? 1 : 0, consumer_state_);
    return BLUSYS_OK;
}

blusys_err_t ble_hid_device_capability::set_battery(std::uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }
    battery_level_ = percent;
    std::printf("[ble_hid_device] battery=%u%%\n",
                static_cast<unsigned>(battery_level_));
    return BLUSYS_OK;
}

}  // namespace blusys

