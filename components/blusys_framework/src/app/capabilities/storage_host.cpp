#ifndef ESP_PLATFORM

#include "blusys/app/capabilities/storage.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

static const char *TAG = "blusys_stor";

namespace blusys::app {

blusys_err_t storage_capability::start(blusys::framework::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;

    if (cfg_.spiffs_base_path != nullptr) {
        status_.spiffs_mounted = true;
    }
    if (cfg_.fatfs_base_path != nullptr) {
        status_.fatfs_mounted = true;
    }

    status_.bundle_ready = true;

    BLUSYS_LOGI(TAG, "host stub: storage ready (simulated)");
    return BLUSYS_OK;
}

void storage_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!first_poll_) {
        return;
    }
    first_poll_ = false;

    if (cfg_.spiffs_base_path != nullptr) {
        post_event(storage_event::spiffs_mounted);
    }
    if (cfg_.fatfs_base_path != nullptr) {
        post_event(storage_event::fatfs_mounted);
    }

    post_event(storage_event::bundle_ready);
}

void storage_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

void storage_capability::post_event(storage_event ev)
{
    if (rt_ != nullptr) {
        rt_->post_event(blusys::framework::make_integration_event(
            static_cast<std::uint32_t>(ev)));
    }
}

}  // namespace blusys::app

#endif  // !ESP_PLATFORM
