// Telemetry capability — same implementation on device and host.
// Transport is product-owned via the delivery callback.

#include "blusys/app/capabilities/telemetry.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

static const char *TAG = "blusys_telem";

namespace blusys::app {

telemetry_capability::telemetry_capability(const telemetry_config &cfg)
    : cfg_(cfg)
{
}

blusys_err_t telemetry_capability::start(blusys::framework::runtime &rt)
{
    rt_ = &rt;
    write_pos_ = 0;
    last_flush_ms_ = 0;
    status_ = {};
    first_poll_ready_ = true;

    BLUSYS_LOGI(TAG, "telemetry starting (threshold=%u, interval=%lums)",
                static_cast<unsigned>(cfg_.flush_threshold),
                static_cast<unsigned long>(cfg_.flush_interval_ms));
    return BLUSYS_OK;
}

void telemetry_capability::poll(std::uint32_t now_ms)
{
    if (first_poll_ready_) {
        first_poll_ready_ = false;
        status_.capability_ready = true;
        post_event(telemetry_event::capability_ready);
    }

    if (write_pos_ == 0) {
        // Update the flush timer even with an empty buffer so the
        // first recorded metric doesn't trigger an immediate flush.
        if (last_flush_ms_ == 0) {
            last_flush_ms_ = now_ms;
        }
        return;
    }

    bool should_flush = false;

    if (write_pos_ >= cfg_.flush_threshold) {
        should_flush = true;
    }
    if (cfg_.flush_interval_ms > 0 &&
        now_ms - last_flush_ms_ >= cfg_.flush_interval_ms) {
        should_flush = true;
    }

    if (should_flush) {
        flush();
        last_flush_ms_ = now_ms;
    }
}

void telemetry_capability::stop()
{
    // Flush remaining metrics before shutdown.
    if (write_pos_ > 0) {
        flush();
    }
    status_ = {};
    first_poll_ready_ = true;
    rt_ = nullptr;
}

bool telemetry_capability::record(const char *name, float value,
                                  std::uint32_t timestamp_ms)
{
    if (write_pos_ >= kMaxTelemetryBuffer) {
        post_event(telemetry_event::buffer_full);
        return false;
    }

    buffer_[write_pos_] = {name, value, timestamp_ms};
    ++write_pos_;
    status_.buffered_count = static_cast<std::uint16_t>(write_pos_);
    return true;
}

void telemetry_capability::flush()
{
    if (write_pos_ == 0 || cfg_.deliver == nullptr) {
        return;
    }

    bool ok = cfg_.deliver(buffer_, write_pos_, cfg_.deliver_ctx);

    if (ok) {
        status_.total_delivered = static_cast<std::uint16_t>(
            status_.total_delivered + write_pos_);
        post_event(telemetry_event::delivery_ok);
    } else {
        status_.total_failed = static_cast<std::uint16_t>(
            status_.total_failed + write_pos_);
        post_event(telemetry_event::delivery_failed);
    }

    write_pos_ = 0;
    status_.buffered_count = 0;
    post_event(telemetry_event::buffer_flushed);
}

}  // namespace blusys::app
