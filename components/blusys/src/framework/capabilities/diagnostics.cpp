#ifdef ESP_PLATFORM

#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_timer.h"
#include "esp_idf_version.h"
#include "esp_flash.h"

#include <cstring>
#include <cstdio>

static const char *TAG = "blusys_diag";

namespace blusys {

diagnostics_capability::diagnostics_capability(const diagnostics_config &cfg)
    : cfg_(cfg)
{
}

blusys_err_t diagnostics_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;

    // Optional console.
    if (cfg_.enable_console) {
        blusys_console_config_t con_cfg{};
        con_cfg.uart_num  = cfg_.console_uart_num;
        con_cfg.baud_rate = cfg_.console_baud_rate;
        con_cfg.tx_pin    = -1;
        con_cfg.rx_pin    = -1;

        blusys_err_t err = blusys_console_open(&con_cfg, &console_);
        if (err == BLUSYS_OK) {
            blusys_console_start(console_);
            status_.console_running = true;
            BLUSYS_LOGI(TAG, "console started on UART%d", cfg_.console_uart_num);
        } else {
            BLUSYS_LOGW(TAG, "console open failed: %d", static_cast<int>(err));
        }
    }

    // Collect initial snapshot.
    collect_snapshot();

    // Defer capability_ready event to first poll.
    first_poll_ = true;
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
    }

    if (cfg_.snapshot_interval_ms == 0) {
        return;
    }

    if (now_ms - last_snapshot_ms_ >= cfg_.snapshot_interval_ms) {
        last_snapshot_ms_ = now_ms;
        collect_snapshot();
        post_event(diagnostics_event::snapshot_ready);
    }
}

void diagnostics_capability::stop()
{
    if (console_ != nullptr) {
        blusys_console_close(console_);
        console_ = nullptr;
    }
    status_ = {};
    last_snapshot_ms_ = 0;
    first_poll_ = true;
    rt_ = nullptr;
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

    // Chip model name.
    const char *model_name = "unknown";
    switch (chip.model) {
    case CHIP_ESP32:   model_name = "ESP32";   break;
    case CHIP_ESP32S3: model_name = "ESP32-S3"; break;
    case CHIP_ESP32C3: model_name = "ESP32-C3"; break;
    default: break;
    }
    std::strncpy(s.chip_model, model_name, sizeof(s.chip_model) - 1);
    s.chip_model[sizeof(s.chip_model) - 1] = '\0';

    // Flash size.
    std::uint32_t flash_size = 0;
    if (esp_flash_get_size(nullptr, &flash_size) == ESP_OK) {
        s.flash_size = flash_size;
    }
}

}  // namespace blusys

#endif  // ESP_PLATFORM
