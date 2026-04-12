#ifndef ESP_PLATFORM

#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

#include <cstring>

static const char *TAG = "blusys_diag";

namespace blusys::app {

blusys_err_t diagnostics_capability::start(blusys::framework::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;
    last_snapshot_ms_ = 0;

    // Populate synthetic snapshot.
    auto &s = status_.last_snapshot;
    s.free_heap     = 262144;
    s.min_free_heap = 196608;
    s.uptime_ms     = 0;
    std::strncpy(s.idf_version, "v5.3-host", sizeof(s.idf_version) - 1);
    std::strncpy(s.chip_model, "Host-x86", sizeof(s.chip_model) - 1);
    s.chip_cores = 4;
    s.flash_size = 4 * 1024 * 1024;

    BLUSYS_LOGI(TAG, "host stub: diagnostics ready (simulated)");
    return BLUSYS_OK;
}

void diagnostics_capability::poll(std::uint32_t now_ms)
{
    if (first_poll_) {
        first_poll_ = false;
        last_snapshot_ms_ = now_ms;
        status_.capability_ready = true;
        post_event(diagnostics_event::snapshot_ready);
        post_event(diagnostics_event::capability_ready);
        return;
    }

    if (cfg_.snapshot_interval_ms > 0 &&
        now_ms - last_snapshot_ms_ >= cfg_.snapshot_interval_ms) {
        last_snapshot_ms_ = now_ms;
        status_.last_snapshot.uptime_ms = now_ms;
        post_event(diagnostics_event::snapshot_ready);
    }
}

void diagnostics_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

}  // namespace blusys::app

#endif  // !ESP_PLATFORM
