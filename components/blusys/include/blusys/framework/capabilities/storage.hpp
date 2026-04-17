#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <cstdint>

// Forward declarations for storage service handles.
// The concrete struct definitions live in the device service headers.
struct blusys_fs;
typedef struct blusys_fs blusys_fs_t;
struct blusys_fatfs;
typedef struct blusys_fatfs blusys_fatfs_t;

namespace blusys { class runtime; }

namespace blusys {

// ---- shared types (always available) ----

enum class storage_event : std::uint32_t {
    spiffs_mounted = 0x0200,
    fatfs_mounted  = 0x0201,
    capability_ready   = 0x02FF,
};

struct storage_status : capability_status_base {
    bool spiffs_mounted = false;
    bool fatfs_mounted  = false;
};

// ---- configuration (platform-independent) ----

struct storage_config {
    const char *spiffs_base_path       = nullptr;
    const char *spiffs_partition_label  = nullptr;
    bool        spiffs_format_on_fail   = true;

    const char *fatfs_base_path        = nullptr;
    const char *fatfs_partition_label   = nullptr;
    bool        fatfs_format_on_fail    = true;
};

// ---- capability class ----

class storage_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::storage;

    explicit storage_capability(const storage_config &cfg);
    ~storage_capability() override;

    [[nodiscard]] capability_kind kind() const override { return capability_kind::storage; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const storage_status &status() const { return status_; }

    // Returns nullptr if the filesystem was not configured or failed to mount.
    [[nodiscard]] blusys_fs_t    *spiffs() const;
    [[nodiscard]] blusys_fatfs_t *fatfs()  const;

private:
    void post_event(storage_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    struct impl;
    impl *impl_ = nullptr;

    storage_config  cfg_;
    storage_status  status_{};
    bool            first_poll_ = true;
};

}  // namespace blusys
