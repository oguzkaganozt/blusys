#ifdef ESP_PLATFORM

#include "blusys/app/bundles/storage.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

static const char *TAG = "blusys_stor";

namespace blusys::app {

storage_bundle::storage_bundle(const storage_config &cfg)
    : cfg_(cfg)
{
}

blusys_err_t storage_bundle::start(blusys::framework::runtime &rt)
{
    rt_ = &rt;

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
        status_.bundle_ready = true;
        post_event(storage_event::bundle_ready);
    }

    return BLUSYS_OK;
}

void storage_bundle::poll(std::uint32_t /*now_ms*/)
{
    // Storage operations are synchronous — nothing to poll.
}

void storage_bundle::stop()
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

void storage_bundle::post_event(storage_event ev)
{
    if (rt_ != nullptr) {
        rt_->post_event(blusys::framework::make_integration_event(
            static_cast<std::uint32_t>(ev)));
    }
}

}  // namespace blusys::app

#endif  // ESP_PLATFORM
