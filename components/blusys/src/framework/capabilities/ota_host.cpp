#ifndef ESP_PLATFORM

#include "blusys/framework/capabilities/ota.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

static const char *TAG = "blusys_ota";

namespace blusys {

blusys_err_t ota_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;

    status_.marked_valid = true;

    BLUSYS_LOGI(TAG, "host stub: OTA ready (simulated)");
    return BLUSYS_OK;
}

void ota_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!first_poll_) {
        return;
    }
    first_poll_ = false;

    status_.capability_ready = true;
    post_event(ota_event::marked_valid, 0);
    post_event(ota_event::capability_ready, 0);
}

void ota_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

blusys_err_t ota_capability::request_update()
{
    return request_update(cfg_.firmware_url, cfg_.cert_pem);
}

blusys_err_t ota_capability::request_update(const char *url, const char * /*cert_pem*/)
{
    BLUSYS_LOGI(TAG, "host stub: simulating OTA update (url=%s)",
                url != nullptr ? url : "<cfg>");
    last_progress_posted_ = 255;
    status_.downloading = true;
    post_event(ota_event::download_started, 0);

    for (std::uint8_t p : {0U, 25U, 50U, 75U, 100U}) {
        emit_download_progress(p);
    }

    status_.downloading = false;
    status_.download_complete = true;
    status_.apply_complete = true;
    post_event(ota_event::download_complete, 0);
    post_event(ota_event::apply_complete, 0);

    return BLUSYS_OK;
}

void ota_capability::emit_download_progress(std::uint8_t pct)
{
    status_.progress_pct = pct;
    if (last_progress_posted_ != 255 && pct < 100 &&
        pct < static_cast<std::uint8_t>(last_progress_posted_ + 5)) {
        return;
    }
    if (last_progress_posted_ == pct && pct < 100) {
        return;
    }
    last_progress_posted_ = pct;
    post_event(ota_event::download_progress, pct);
}

}  // namespace blusys

#endif  // !ESP_PLATFORM
