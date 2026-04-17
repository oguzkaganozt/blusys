#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

#include "nvs_flash.h"

static const char *TAG = "blusys_stor";

namespace blusys {

storage_capability::storage_capability(const storage_config &cfg)
    : cfg_(cfg)
{
}

blusys_err_t storage_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;

    if (cfg_.init_nvs) {
        esp_err_t nvs_err = nvs_flash_init();
        if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
            nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            nvs_err = nvs_flash_init();
        }
        if (nvs_err != ESP_OK) {
            BLUSYS_LOGE(TAG, "NVS init failed: %d", nvs_err);
            return BLUSYS_ERR_INTERNAL;
        }
    }

    // SPIFFS
    if (cfg_.spiffs_base_path != nullptr) {
        blusys_fs_config_t fs_cfg{};
        fs_cfg.base_path              = cfg_.spiffs_base_path;
        fs_cfg.partition_label        = cfg_.spiffs_partition_label;
        fs_cfg.format_if_mount_failed = cfg_.spiffs_format_on_fail;

        blusys_err_t err = blusys_fs_mount(&fs_cfg, &spiffs_);
        if (err == BLUSYS_OK) {
            status_.spiffs_mounted = true;
            post_event(storage_event::spiffs_mounted);
            BLUSYS_LOGI(TAG, "SPIFFS mounted at %s", cfg_.spiffs_base_path);
        } else {
            BLUSYS_LOGW(TAG, "SPIFFS mount failed: %d", static_cast<int>(err));
        }
    }

    // FATFS
    if (cfg_.fatfs_base_path != nullptr) {
        blusys_fatfs_config_t fat_cfg{};
        fat_cfg.base_path              = cfg_.fatfs_base_path;
        fat_cfg.partition_label        = cfg_.fatfs_partition_label;
        fat_cfg.format_if_mount_failed = cfg_.fatfs_format_on_fail;

        blusys_err_t err = blusys_fatfs_mount(&fat_cfg, &fatfs_);
        if (err == BLUSYS_OK) {
            status_.fatfs_mounted = true;
            post_event(storage_event::fatfs_mounted);
            BLUSYS_LOGI(TAG, "FATFS mounted at %s", cfg_.fatfs_base_path);
        } else {
            BLUSYS_LOGW(TAG, "FATFS mount failed: %d", static_cast<int>(err));
        }
    }

    // Check if everything that was requested is now up.
    bool ready = true;
    if (cfg_.spiffs_base_path != nullptr && !status_.spiffs_mounted) {
        ready = false;
    }
    if (cfg_.fatfs_base_path != nullptr && !status_.fatfs_mounted) {
        ready = false;
    }

    if (ready) {
        status_.capability_ready = true;
        post_event(storage_event::capability_ready);
    }

    return BLUSYS_OK;
}

void storage_capability::poll(std::uint32_t /*now_ms*/)
{
    // Storage operations are synchronous — nothing to poll.
}

void storage_capability::stop()
{
    if (fatfs_ != nullptr) {
        blusys_fatfs_unmount(fatfs_);
        fatfs_ = nullptr;
    }
    if (spiffs_ != nullptr) {
        blusys_fs_unmount(spiffs_);
        spiffs_ = nullptr;
    }

    status_ = {};
    rt_ = nullptr;
}

}  // namespace blusys

