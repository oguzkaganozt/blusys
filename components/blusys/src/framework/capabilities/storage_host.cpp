#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

static const char *TAG = "blusys_stor";

namespace blusys {

blusys_err_t storage_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;

    (void)cfg_.init_nvs;  // no-op on host — NVS is device-only

    if (cfg_.spiffs_base_path != nullptr) {
        status_.spiffs_mounted = true;
    }
    if (cfg_.fatfs_base_path != nullptr) {
        status_.fatfs_mounted = true;
    }

    status_.capability_ready = true;

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

    post_event(storage_event::capability_ready);
}

void storage_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

}  // namespace blusys

