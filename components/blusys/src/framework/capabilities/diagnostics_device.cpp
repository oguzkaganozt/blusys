#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/observe/log.h"
#include "blusys/observe/snapshot.h"
#include "blusys/hal/nvs.h"
#include "blusys/framework/services/system.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_idf_version.h"
#include "esp_system.h"
#include "esp_timer.h"

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
constexpr const char   *kCrashDumpNs      = "blusys.obs";
constexpr const char   *kCrashDumpKey     = "crash_v1";

diagnostics_capability *g_active_diagnostics = nullptr;

}  // namespace

struct diagnostics_capability::impl {
    blusys_console_t *console = nullptr;
    blusys_nvs_t     *crash_nvs = nullptr;
};

void diagnostics_shutdown_handler()
{
    if (g_active_diagnostics != nullptr) {
        g_active_diagnostics->persist_crash_dump();
    }
}

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
    status_.crash_dump_ready = false;

    blusys_nvs_t *nvs = nullptr;
    if (blusys_nvs_open(kCrashDumpNs, BLUSYS_NVS_READWRITE, &nvs) != BLUSYS_OK) {
        return;
    }

    impl_->crash_nvs = nvs;

    crash_dump_record record{};
    std::size_t len = sizeof(record);
    blusys_err_t err = blusys_nvs_get_blob(nvs, kCrashDumpKey, &record, &len);
    if (err == BLUSYS_OK && len == sizeof(record) &&
        record.magic == kCrashDumpMagic && record.version == kCrashDumpVersion) {
        status_.crash_dump = record.snapshot;
        status_.crash_dump_ready = true;
    }
}

void diagnostics_capability::persist_crash_dump()
{
    if (impl_->crash_nvs == nullptr) {
        return;
    }

    capture_observability();

    crash_dump_record record{};
    record.magic   = kCrashDumpMagic;
    record.version = kCrashDumpVersion;
    record.snapshot = status_.observability;

    if (blusys_nvs_set_blob(impl_->crash_nvs, kCrashDumpKey, &record, sizeof(record)) == BLUSYS_OK) {
        blusys_nvs_commit(impl_->crash_nvs);
    }
}

void diagnostics_capability::collect_snapshot()
{
    auto &s = status_.last_snapshot;

    s.free_heap     = static_cast<std::uint32_t>(esp_get_free_heap_size());
    s.min_free_heap = static_cast<std::uint32_t>(esp_get_minimum_free_heap_size());
    s.uptime_ms     = static_cast<std::uint32_t>(esp_timer_get_time() / 1000);

    std::strncpy(s.idf_version, esp_get_idf_version(), sizeof(s.idf_version) - 1);
    s.idf_version[sizeof(s.idf_version) - 1] = '\0';

    esp_chip_info_t chip{};
    esp_chip_info(&chip);
    s.chip_cores = static_cast<std::uint8_t>(chip.cores);

    const char *model_name = "unknown";
    switch (chip.model) {
    case CHIP_ESP32:   model_name = "ESP32";    break;
    case CHIP_ESP32S3: model_name = "ESP32-S3"; break;
    case CHIP_ESP32C3: model_name = "ESP32-C3"; break;
    default: break;
    }
    std::strncpy(s.chip_model, model_name, sizeof(s.chip_model) - 1);
    s.chip_model[sizeof(s.chip_model) - 1] = '\0';

    std::uint32_t flash_size = 0;
    if (esp_flash_get_size(nullptr, &flash_size) == ESP_OK) {
        s.flash_size = flash_size;
    }
}

blusys_err_t diagnostics_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    g_active_diagnostics = this;

    if (cfg_.enable_console) {
        blusys_console_config_t con_cfg{};
        con_cfg.uart_num  = cfg_.console_uart_num;
        con_cfg.baud_rate = cfg_.console_baud_rate;
        con_cfg.tx_pin    = -1;
        con_cfg.rx_pin    = -1;

        blusys_err_t err = blusys_console_open(&con_cfg, &impl_->console);
        if (err == BLUSYS_OK) {
            blusys_console_start(impl_->console);
            status_.console_running = true;
            BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_framework_observe,
                       "console started on UART%d", cfg_.console_uart_num);
        } else {
            BLUSYS_LOG(BLUSYS_LOG_WARN, err_domain_framework_observe,
                       "console open failed: %d", static_cast<int>(err));
        }
    }

    collect_snapshot();
    load_crash_dump();
    status_.capability_ready = true;
    first_poll_ = true;
    capture_observability();

    esp_register_shutdown_handler(&diagnostics_shutdown_handler);

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
    }

    if (cfg_.snapshot_interval_ms == 0) {
        return;
    }

    if (now_ms - last_snapshot_ms_ >= cfg_.snapshot_interval_ms) {
        last_snapshot_ms_ = now_ms;
        collect_snapshot();
        capture_observability();
        post_event(diagnostics_event::snapshot_ready);
    }
}

void diagnostics_capability::stop()
{
    g_active_diagnostics = nullptr;
    persist_crash_dump();

    if (impl_->console != nullptr) {
        blusys_console_close(impl_->console);
        impl_->console = nullptr;
    }
    if (impl_->crash_nvs != nullptr) {
        blusys_nvs_close(impl_->crash_nvs);
        impl_->crash_nvs = nullptr;
    }

    status_ = {};
    last_snapshot_ms_ = 0;
    first_poll_ = true;
    rt_ = nullptr;
}

}  // namespace blusys
