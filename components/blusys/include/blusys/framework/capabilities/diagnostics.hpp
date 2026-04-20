#pragma once

#include "blusys/framework/capabilities/capability.hpp"
#include "blusys/observe/snapshot.h"

#include <cstdint>

namespace blusys { class runtime; }

namespace blusys {

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

struct diagnostics_status : capability_status_base {
    bool console_running = false;
    diagnostics_snapshot last_snapshot{};
    // Live observability snapshot: log ring, counters, and last-error state.
    blusys_observe_snapshot_t observability{};
    // Restored crash dump from the previous boot/shutdown capture, if any.
    bool crash_dump_ready = false;
    blusys_observe_snapshot_t crash_dump{};
};

// ---- configuration (platform-independent) ----

struct diagnostics_config {
    bool enable_console              = false;
    int  console_uart_num            = 0;
    int  console_baud_rate           = 115200;
    std::uint32_t snapshot_interval_ms = 5000;
};

// ---- capability class ----

class diagnostics_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::diagnostics;

    explicit diagnostics_capability(const diagnostics_config &cfg);
    ~diagnostics_capability() override;

    [[nodiscard]] capability_kind kind() const override { return capability_kind::diagnostics; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const diagnostics_status &status() const { return status_; }

    struct effects {
        diagnostics_capability *self = nullptr;

        [[nodiscard]] const diagnostics_status *status() const;
        [[nodiscard]] const diagnostics_snapshot *snapshot() const;
        [[nodiscard]] const blusys_observe_snapshot_t *observability() const;
        [[nodiscard]] const blusys_observe_snapshot_t *crash_dump() const;
        [[nodiscard]] const blusys_observe_last_error_t *last_error(blusys_err_domain_t domain) const;
        [[nodiscard]] std::uint32_t counter(blusys_err_domain_t domain) const;
        void bind(diagnostics_capability *diag) noexcept;
    };

private:
    friend void diagnostics_shutdown_handler();

    void collect_snapshot();
    void capture_observability();
    void load_crash_dump();
    void persist_crash_dump();
    void post_event(diagnostics_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    struct impl;
    impl *impl_ = nullptr;

    diagnostics_config cfg_;
    diagnostics_status status_{};
    bool           first_poll_        = true;
    std::uint32_t  last_snapshot_ms_  = 0;
};

// ---- effects inline bodies ----

inline const diagnostics_status *diagnostics_capability::effects::status() const
{
    return self != nullptr ? &self->status() : nullptr;
}

inline const diagnostics_snapshot *diagnostics_capability::effects::snapshot() const
{
    const auto *s = status();
    return s != nullptr ? &s->last_snapshot : nullptr;
}

inline const blusys_observe_snapshot_t *diagnostics_capability::effects::observability() const
{
    const auto *s = status();
    return s != nullptr ? &s->observability : nullptr;
}

inline const blusys_observe_snapshot_t *diagnostics_capability::effects::crash_dump() const
{
    const auto *s = status();
    return s != nullptr && s->crash_dump_ready ? &s->crash_dump : nullptr;
}

inline const blusys_observe_last_error_t *diagnostics_capability::effects::last_error(blusys_err_domain_t domain) const
{
    const auto *s = status();
    if (s == nullptr || (unsigned)domain >= (unsigned)err_domain_count) {
        return nullptr;
    }
    const auto &e = s->observability.last_errors[domain];
    return e.valid ? &e : nullptr;
}

inline std::uint32_t diagnostics_capability::effects::counter(blusys_err_domain_t domain) const
{
    const auto *s = status();
    if (s == nullptr || (unsigned)domain >= (unsigned)err_domain_count) {
        return 0;
    }
    return s->observability.counters.values[domain];
}

inline void diagnostics_capability::effects::bind(diagnostics_capability *diag) noexcept
{
    self = diag;
}

}  // namespace blusys
