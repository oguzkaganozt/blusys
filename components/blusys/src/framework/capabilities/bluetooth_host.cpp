#ifndef ESP_PLATFORM

#include "blusys/framework/capabilities/bluetooth.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

static const char *TAG = "blusys_bt";

namespace blusys {

blusys_err_t bluetooth_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;

    status_.gap_ready = true;
    status_.gatt_ready = true;
    status_.advertising = true;

    BLUSYS_LOGI(TAG, "host stub: bluetooth ready (simulated)");
    return BLUSYS_OK;
}

void bluetooth_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!first_poll_) {
        return;
    }
    first_poll_ = false;

    status_.capability_ready = true;
    post_event(bluetooth_event::gap_ready);
    post_event(bluetooth_event::gatt_ready);
    post_event(bluetooth_event::advertising_started);
    post_event(bluetooth_event::capability_ready);
}

void bluetooth_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

}  // namespace blusys

#endif  // !ESP_PLATFORM
