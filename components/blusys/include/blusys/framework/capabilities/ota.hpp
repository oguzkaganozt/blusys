#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <cstddef>
#include <cstdint>
#include <atomic>

#ifdef ESP_PLATFORM
#include "blusys/services/system/ota.h"
#endif

namespace blusys { class runtime; }

namespace blusys {

// ---- shared types (always available) ----

enum class ota_event : std::uint32_t {
    check_started     = 0x0400,
    download_started  = 0x0401,
    download_progress = 0x0402,
    download_complete = 0x0403,
    download_failed   = 0x0404,
    apply_complete    = 0x0405,
    apply_failed      = 0x0406,
    rollback_pending  = 0x0410,
    marked_valid      = 0x0411,
    capability_ready      = 0x04FF,
};

struct ota_status : capability_status_base {
    bool    downloading      = false;
    bool    download_complete = false;
    bool    apply_complete   = false;
    bool    rollback_pending = false;
    bool    marked_valid     = false;
    std::uint8_t progress_pct = 0;
};

// ---- device implementation ----

#ifdef ESP_PLATFORM

struct ota_config {
    const char *firmware_url   = nullptr;
    const char *cert_pem       = nullptr;
    int         timeout_ms     = 30000;
    bool        auto_mark_valid = true;
    // Optional SHA256 over the full firmware image, 64 ASCII hex chars.
    // nullptr skips verification. When the URL is pushed at runtime, the
    // runtime-supplied checksum in request_update() takes precedence.
    const char *expected_sha256 = nullptr;
};

class ota_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::ota;

    explicit ota_capability(const ota_config &cfg);

    [[nodiscard]] capability_kind kind() const override { return capability_kind::ota; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const ota_status &status() const { return status_; }

    // Product calls this from system/ in response to a reducer action.
    // Blocks until the OTA download + flash completes or fails. Uses
    // `cfg_.firmware_url` (and `cfg_.cert_pem`) as the source.
    blusys_err_t request_update();

    // Dynamic-URL variant — used when the firmware URL and (optionally)
    // certificate are handed to the device at runtime, e.g. pushed over
    // MQTT by Atlas OTA announcements. `cert_pem == nullptr` falls back
    // to `cfg_.cert_pem` (often the same broker CA bundle is reused for
    // HTTPS). `expected_sha256` is a 64-char hex digest; nullptr falls
    // back to `cfg_.expected_sha256`. Does not mutate `cfg_`.
    blusys_err_t request_update(const char *url,
                                const char *cert_pem        = nullptr,
                                const char *expected_sha256 = nullptr);

    // Used by the services-layer OTA progress callback (not a general app API).
    void emit_download_progress(std::uint8_t pct);

private:
    static constexpr std::uint32_t kPendingNone          = 0;
    static constexpr std::uint32_t kPendingApplyComplete  = 1 << 0;
    static constexpr std::uint32_t kPendingApplyFailed    = 1 << 1;
    static constexpr std::uint32_t kPendingMarkedValid    = 1 << 2;
    static constexpr std::uint32_t kPendingRollback       = 1 << 3;
    static constexpr std::uint32_t kPendingCapabilityReady    = 1 << 4;
    static constexpr std::uint32_t kPendingDownloadStarted = 1 << 5;

    void post_event(ota_event ev, std::uint32_t code = 0)
    {
        post_integration_event(static_cast<std::uint32_t>(ev), code);
    }

    ota_config cfg_;
    ota_status status_{};
    std::atomic<std::uint32_t> pending_flags_{kPendingNone};
    std::uint8_t last_progress_posted_ = 255;
};

#else  // host stub

struct ota_config {
    const char *firmware_url    = nullptr;
    const char *cert_pem        = nullptr;
    int         timeout_ms      = 30000;
    bool        auto_mark_valid = true;
    const char *expected_sha256 = nullptr;
};

class ota_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::ota;

    explicit ota_capability(const ota_config &cfg)
        : cfg_(cfg) {}

    [[nodiscard]] capability_kind kind() const override { return capability_kind::ota; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const ota_status &status() const { return status_; }

    blusys_err_t request_update();
    blusys_err_t request_update(const char *url,
                                const char *cert_pem        = nullptr,
                                const char *expected_sha256 = nullptr);

    void emit_download_progress(std::uint8_t pct);

private:
    void post_event(ota_event ev, std::uint32_t code = 0)
    {
        post_integration_event(static_cast<std::uint32_t>(ev), code);
    }

    ota_config cfg_;
    ota_status status_{};
    bool first_poll_ = true;
    std::uint8_t last_progress_posted_ = 255;
};

#endif  // ESP_PLATFORM

}  // namespace blusys
