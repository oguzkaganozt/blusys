#pragma once

#include "blusys/app/capability.hpp"

#include <cstddef>
#include <cstdint>

#ifdef ESP_PLATFORM
#include "blusys/system/console.h"
#endif

namespace blusys::framework { class runtime; }

namespace blusys::app {

// ---- shared types (always available) ----

enum class diagnostics_event : std::uint32_t {
    snapshot_ready = 0x0500,
    console_ready  = 0x0501,
    capability_ready   = 0x05FF,
};

struct diagnostics_snapshot {
    std::uint32_t free_heap     = 0;
    std::uint32_t min_free_heap = 0;
    std::uint32_t uptime_ms     = 0;
    char idf_version[16]        = {};
    char chip_model[16]         = {};
    std::uint8_t  chip_cores    = 0;
    std::uint32_t flash_size    = 0;
};

struct diagnostics_status {
    bool console_running = false;
    bool capability_ready    = false;
    diagnostics_snapshot last_snapshot{};
};

// ---- device implementation ----

#ifdef ESP_PLATFORM

struct diagnostics_config {
    bool enable_console         = false;
    int  console_uart_num       = 0;
    int  console_baud_rate      = 115200;
    std::uint32_t snapshot_interval_ms = 5000;
};

class diagnostics_capability final : public capability_base {
public:
    explicit diagnostics_capability(const diagnostics_config &cfg);

    [[nodiscard]] capability_kind kind() const override { return capability_kind::diagnostics; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const diagnostics_status &status() const { return status_; }

private:
    void collect_snapshot();
    void post_event(diagnostics_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    diagnostics_config cfg_;
    diagnostics_status status_{};
    blusys_console_t *console_ = nullptr;
    std::uint32_t last_snapshot_ms_ = 0;
    bool first_poll_ = true;
};

#else  // host stub

struct diagnostics_config {
    bool enable_console         = false;
    int  console_uart_num       = 0;
    int  console_baud_rate      = 115200;
    std::uint32_t snapshot_interval_ms = 5000;
};

class diagnostics_capability final : public capability_base {
public:
    explicit diagnostics_capability(const diagnostics_config &cfg)
        : cfg_(cfg) {}

    [[nodiscard]] capability_kind kind() const override { return capability_kind::diagnostics; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const diagnostics_status &status() const { return status_; }

private:
    void post_event(diagnostics_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    diagnostics_config cfg_;
    diagnostics_status status_{};
    bool first_poll_ = true;
    std::uint32_t last_snapshot_ms_ = 0;
};

#endif  // ESP_PLATFORM

}  // namespace blusys::app
