#pragma once

#include "blusys/framework/capabilities/capability.hpp"
#include "blusys/framework/platform/build.hpp"

namespace blusys {

#ifndef BLUSYS_BUILD_HOST
#define BLUSYS_BUILD_HOST "unknown"
#endif

#ifndef BLUSYS_BUILD_COMMIT
#define BLUSYS_BUILD_COMMIT "unknown"
#endif

#ifndef BLUSYS_BUILD_FEATURE_FLAGS
#define BLUSYS_BUILD_FEATURE_FLAGS "unknown"
#endif

struct build_info_status : capability_status_base {
    const char *firmware_version = nullptr;
    const char *build_host       = nullptr;
    const char *commit_hash      = nullptr;
    const char *feature_flags    = nullptr;
};

inline build_info_status make_build_info_status()
{
    build_info_status status{};
    status.firmware_version = platform::version_string();
    status.build_host       = BLUSYS_BUILD_HOST;
    status.commit_hash      = BLUSYS_BUILD_COMMIT;
    status.feature_flags    = BLUSYS_BUILD_FEATURE_FLAGS;
    return status;
}

class build_info_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::build_info;

    build_info_capability();
    ~build_info_capability() override;

    capability_kind kind() const override { return kind_value; }
    blusys_err_t start(runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    const build_info_status &status() const { return status_; }

    struct effects {
        build_info_capability *self = nullptr;

        [[nodiscard]] const build_info_status *status() const
        {
            return self != nullptr ? &self->status() : nullptr;
        }

        [[nodiscard]] const char *firmware_version() const
        {
            const auto *s = status();
            return s != nullptr && s->firmware_version != nullptr ? s->firmware_version : "unknown";
        }

        [[nodiscard]] const char *build_host() const
        {
            const auto *s = status();
            return s != nullptr && s->build_host != nullptr ? s->build_host : "unknown";
        }

        [[nodiscard]] const char *commit_hash() const
        {
            const auto *s = status();
            return s != nullptr && s->commit_hash != nullptr ? s->commit_hash : "unknown";
        }

        [[nodiscard]] const char *feature_flags() const
        {
            const auto *s = status();
            return s != nullptr && s->feature_flags != nullptr ? s->feature_flags : "unknown";
        }

        void bind(build_info_capability *build) noexcept
        {
            self = build;
        }
    };

private:
    build_info_status status_{};
};

}  // namespace blusys
