#ifndef ESP_PLATFORM

#include "blusys/framework/capabilities/usb.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

static const char *TAG = "blusys_usb_cap";

namespace blusys {

blusys_err_t usb_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;
    status_.role_active = false;
    status_.capability_ready = false;

    BLUSYS_LOGI(TAG, "host stub: usb not supported on SDL host");
    return BLUSYS_ERR_NOT_SUPPORTED;
}

void usb_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!first_poll_) {
        return;
    }
    first_poll_ = false;
}

void usb_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

}  // namespace blusys

#endif  // !ESP_PLATFORM
