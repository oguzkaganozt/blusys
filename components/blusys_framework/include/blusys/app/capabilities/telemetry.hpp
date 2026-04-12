#pragma once

#include "blusys/app/capability.hpp"

#include <cstddef>
#include <cstdint>

namespace blusys::framework { class runtime; }

namespace blusys::app {

// ---- shared types (always available) ----

enum class telemetry_event : std::uint32_t {
    buffer_flushed  = 0x0600,
    buffer_full     = 0x0601,
    delivery_ok     = 0x0602,
    delivery_failed = 0x0603,
    capability_ready    = 0x06FF,
};

struct telemetry_metric {
    const char   *name         = nullptr;
    float         value        = 0.0f;
    std::uint32_t timestamp_ms = 0;
};

struct telemetry_status {
    std::uint16_t buffered_count  = 0;
    std::uint16_t total_delivered = 0;
    std::uint16_t total_failed    = 0;
    bool          capability_ready    = false;
};

// User-provided delivery callback. Called from poll() when the buffer
// reaches flush_threshold or flush_interval_ms elapses.
// Return true on successful delivery, false on failure.
using telemetry_deliver_fn = bool (*)(const telemetry_metric *metrics,
                                      std::size_t count,
                                      void *user_ctx);

static constexpr std::size_t kMaxTelemetryBuffer = 32;

struct telemetry_config {
    telemetry_deliver_fn deliver       = nullptr;
    void                *deliver_ctx   = nullptr;
    std::uint16_t flush_threshold      = 16;
    std::uint32_t flush_interval_ms    = 30000;
};

// ---- implementation (same on device and host) ----

class telemetry_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::telemetry;

    explicit telemetry_capability(const telemetry_config &cfg);

    [[nodiscard]] capability_kind kind() const override { return capability_kind::telemetry; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const telemetry_status &status() const { return status_; }

    // Record a metric into the buffer. Returns false if the buffer is full.
    bool record(const char *name, float value, std::uint32_t timestamp_ms = 0);

private:
    void flush();
    void post_event(telemetry_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    telemetry_config  cfg_;
    telemetry_status  status_{};
    telemetry_metric  buffer_[kMaxTelemetryBuffer]{};
    std::size_t       write_pos_   = 0;
    std::uint32_t     last_flush_ms_ = 0;
    bool              first_poll_ready_ = true;
};

}  // namespace blusys::app
