#pragma once

#include "blusys/app/capability.hpp"

#include <cstdint>

#ifdef ESP_PLATFORM
#include "blusys/system/fatfs.h"
#include "blusys/system/fs.h"
#endif

namespace blusys::framework { class runtime; }

namespace blusys::app {

// ---- shared types (always available) ----

// Well-known integration event IDs posted by the storage capability.
enum class storage_event : std::uint32_t {
    spiffs_mounted = 0x0200,
    fatfs_mounted  = 0x0201,
    capability_ready   = 0x02FF,
};

// Current storage state, queryable via ctx.storage().
struct storage_status {
    bool spiffs_mounted = false;
    bool fatfs_mounted  = false;
    bool capability_ready   = false;
};

// Declarative storage configuration.
// Each filesystem is optional — enabled when its base_path is non-null.
struct storage_config {
    // When true, runs `nvs_flash_init()` before mounting filesystems. Use when
    // the app does not compose `connectivity_capability` (which also inits NVS).
    bool init_nvs = false;

    const char *spiffs_base_path       = nullptr;
    const char *spiffs_partition_label  = nullptr;
    bool        spiffs_format_on_fail   = true;

    const char *fatfs_base_path        = nullptr;
    const char *fatfs_partition_label   = nullptr;
    bool        fatfs_format_on_fail    = true;
};

// ---- device implementation ----

#ifdef ESP_PLATFORM

class storage_capability final : public capability_base {
public:
    explicit storage_capability(const storage_config &cfg);

    [[nodiscard]] capability_kind kind() const override { return capability_kind::storage; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const storage_status &status() const { return status_; }

    // Direct handle access for product code.
    // Returns nullptr if the filesystem was not configured or failed to mount.
    [[nodiscard]] blusys_fs_t    *spiffs() const { return spiffs_; }
    [[nodiscard]] blusys_fatfs_t *fatfs()  const { return fatfs_; }

private:
    void post_event(storage_event ev);

    storage_config   cfg_;
    storage_status   status_{};
    blusys::framework::runtime *rt_ = nullptr;

    blusys_fs_t      *spiffs_ = nullptr;
    blusys_fatfs_t   *fatfs_  = nullptr;
};

#else  // host stub

class storage_capability final : public capability_base {
public:
    explicit storage_capability(const storage_config &cfg)
        : cfg_(cfg) {}

    [[nodiscard]] capability_kind kind() const override { return capability_kind::storage; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const storage_status &status() const { return status_; }

private:
    void post_event(storage_event ev);

    storage_config   cfg_;
    storage_status   status_{};
    blusys::framework::runtime *rt_ = nullptr;
    bool first_poll_ = true;
};

#endif  // ESP_PLATFORM

}  // namespace blusys::app
