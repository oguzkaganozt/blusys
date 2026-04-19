#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/events/event.hpp"
#include "blusys/framework/events/event_queue.hpp"
#include "blusys/hal/log.h"
#include "blusys/framework/services/storage.h"

static const char *TAG = "blusys_stor";

namespace blusys {

struct storage_capability::impl {
    blusys_fs_t    *spiffs = nullptr;
    blusys_fatfs_t *fatfs  = nullptr;
};

storage_capability::storage_capability(const storage_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

storage_capability::~storage_capability()
{
    delete impl_;
}

blusys_fs_t    *storage_capability::spiffs() const { return impl_->spiffs; }
blusys_fatfs_t *storage_capability::fatfs()  const { return impl_->fatfs;  }

blusys_err_t storage_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;

    // SPIFFS
    if (cfg_.spiffs_base_path != nullptr) {
        blusys_fs_config_t fs_cfg{};
        fs_cfg.base_path              = cfg_.spiffs_base_path;
        fs_cfg.partition_label        = cfg_.spiffs_partition_label;
        fs_cfg.format_if_mount_failed = cfg_.spiffs_format_on_fail;

        blusys_err_t err = blusys_fs_mount(&fs_cfg, &impl_->spiffs);
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

        blusys_err_t err = blusys_fatfs_mount(&fat_cfg, &impl_->fatfs);
        if (err == BLUSYS_OK) {
            status_.fatfs_mounted = true;
            post_event(storage_event::fatfs_mounted);
            BLUSYS_LOGI(TAG, "FATFS mounted at %s", cfg_.fatfs_base_path);
        } else {
            BLUSYS_LOGW(TAG, "FATFS mount failed: %d", static_cast<int>(err));
        }
    }

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
    if (impl_->fatfs != nullptr) {
        blusys_fatfs_unmount(impl_->fatfs);
        impl_->fatfs = nullptr;
    }
    if (impl_->spiffs != nullptr) {
        blusys_fs_unmount(impl_->spiffs);
        impl_->spiffs = nullptr;
    }

    status_ = {};
    rt_ = nullptr;
}

}  // namespace blusys
