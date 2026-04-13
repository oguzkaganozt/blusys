#ifndef ESP_PLATFORM

#include "blusys/app/capabilities/usb.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

static const char *TAG = "blusys_usb_cap";

namespace blusys::app {

blusys_err_t usb_capability::start(blusys::framework::runtime &rt)
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

}  // namespace blusys::app

#endif  // !ESP_PLATFORM
