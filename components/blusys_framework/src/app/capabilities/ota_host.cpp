#ifndef ESP_PLATFORM

#include "blusys/app/capabilities/ota.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

static const char *TAG = "blusys_ota";

namespace blusys::app {

blusys_err_t ota_capability::start(blusys::framework::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;

    status_.marked_valid = true;
    status_.capability_ready = true;

    BLUSYS_LOGI(TAG, "host stub: OTA ready (simulated)");
    return BLUSYS_OK;
}

void ota_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!first_poll_) {
        return;
    }
    first_poll_ = false;

    post_event(ota_event::marked_valid);
    post_event(ota_event::capability_ready);
}

void ota_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

blusys_err_t ota_capability::request_update()
{
    BLUSYS_LOGI(TAG, "host stub: simulating OTA update");
    status_.downloading = true;
    post_event(ota_event::download_started);

    // Simulate immediate success.
    status_.downloading = false;
    status_.download_complete = true;
    status_.apply_complete = true;
    post_event(ota_event::download_complete);
    post_event(ota_event::apply_complete);

    return BLUSYS_OK;
}

void ota_capability::post_event(ota_event ev)
{
    if (rt_ != nullptr) {
        rt_->post_event(blusys::framework::make_integration_event(
            static_cast<std::uint32_t>(ev)));
    }
}

}  // namespace blusys::app

#endif  // !ESP_PLATFORM
