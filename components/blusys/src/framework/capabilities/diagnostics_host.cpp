#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/observe/log.h"
#include "blusys/framework/observe/snapshot.h"

#include <cstring>

namespace blusys {

namespace {

struct crash_dump_record {
    std::uint32_t magic   = 0;
    std::uint32_t version = 0;
    blusys_observe_snapshot_t snapshot{};
};

constexpr std::uint32_t kCrashDumpMagic   = 0x42444947u;  // BDIG
constexpr std::uint32_t kCrashDumpVersion = 1u;

crash_dump_record g_host_crash_dump{};
bool              g_host_crash_dump_valid = false;

}  // namespace

struct diagnostics_capability::impl {};

diagnostics_capability::diagnostics_capability(const diagnostics_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

diagnostics_capability::~diagnostics_capability()
{
    delete impl_;
}

void diagnostics_capability::capture_observability()
{
    blusys_observe_snapshot(&status_.observability);
}

void diagnostics_capability::load_crash_dump()
{
    if (!g_host_crash_dump_valid || g_host_crash_dump.magic != kCrashDumpMagic ||
        g_host_crash_dump.version != kCrashDumpVersion) {
        status_.crash_dump_ready = false;
        return;
    }

    status_.crash_dump = g_host_crash_dump.snapshot;
    status_.crash_dump_ready = true;
}

void diagnostics_capability::persist_crash_dump()
{
    capture_observability();
    g_host_crash_dump.magic   = kCrashDumpMagic;
    g_host_crash_dump.version = kCrashDumpVersion;
    g_host_crash_dump.snapshot = status_.observability;
    g_host_crash_dump_valid = true;
}

void diagnostics_capability::collect_snapshot() {}

blusys_err_t diagnostics_capability::start(blusys::runtime &rt)
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

    load_crash_dump();
    status_.capability_ready = true;

    BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_framework_observe,
               "host stub: diagnostics ready (simulated)");
    capture_observability();
    return BLUSYS_OK;
}

void diagnostics_capability::poll(std::uint32_t now_ms)
{
    capture_observability();

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
    persist_crash_dump();
    status_ = {};
    rt_ = nullptr;
}

}  // namespace blusys
