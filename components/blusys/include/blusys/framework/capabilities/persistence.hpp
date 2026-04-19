#pragma once

#include "blusys/framework/capabilities/capability.hpp"
#include "blusys/hal/nvs.h"

#include <cstddef>
#include <cstdint>

namespace blusys {

struct capability_list;  // full type in capabilities/list.hpp

// Optional mixin for capabilities that declare versioned persistent state.
// A capability inherits capability_persistence_schema alongside capability_base
// to opt into automatic schema versioning and migration on boot.
//
// The framework discovers schema-capable capabilities without RTTI via
// capability_base::as_persistence_schema(). Any capability using this mixin
// must also override as_persistence_schema() to return `this`:
//
//   capability_persistence_schema* as_persistence_schema() noexcept override { return this; }
//
// Example (see docs/app/adding-a-capability.md for the full template):
//
//   class my_capability final : public capability_base,
//                               public capability_persistence_schema {
//     ...
//     capability_persistence_schema* as_persistence_schema() noexcept override { return this; }
//     const char* persistence_namespace() const override { return "my_cap"; }
//     uint32_t    schema_version()        const override { return 1; }
//   };
class capability_persistence_schema {
public:
    virtual ~capability_persistence_schema() = default;

    // NVS namespace this capability owns. Max 15 chars (ESP-IDF limit).
    virtual const char* persistence_namespace() const = 0;

    // Monotonically increasing schema version. Increment when stored format
    // changes. The framework persists this in a dedicated meta namespace and
    // detects upgrades automatically.
    virtual uint32_t schema_version() const = 0;

    // Called when stored version differs from current schema_version(). The
    // supplied nvs handle is opened read-write on the capability's own
    // namespace. On host builds nvs may be nullptr; handle gracefully.
    // Default: no migration required (existing data is compatible).
    virtual blusys_err_t migrate_from(uint32_t prev_version, blusys_nvs_t* nvs)
    {
        (void)prev_version;
        (void)nvs;
        return BLUSYS_OK;
    }
};

// ---- events (0x0D00 – 0x0DFF) ----

enum class persistence_event : std::uint32_t {
    ready            = 0x0D00,  // NVS init done; migrations complete
    migration_done   = 0x0D01,  // one capability migration succeeded
    migration_failed = 0x0D02,  // one capability migration failed
};

// ---- status ----

struct persistence_status : capability_status_base {
    bool     nvs_ready      = false;
    uint32_t migrations_run = 0;
};

// ---- config ----

struct persistence_config {
    // NVS namespace for fx.settings.* app-level keys. Max 15 chars.
    // nullptr → "blusys_app"
    const char* app_namespace = nullptr;
};

// ---- capability class ----

class persistence_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::persistence;

    explicit persistence_capability(const persistence_config& cfg = {});
    ~persistence_capability() override;

    capability_kind kind() const override { return kind_value; }

    // Initializes NVS flash. Called by the framework before other capabilities.
    blusys_err_t start(runtime& rt) override;
    void         poll(std::uint32_t now_ms) override;
    void         stop() override;

    const persistence_status& status() const { return status_; }

    // Scan `list` for capabilities that implement capability_persistence_schema.
    // For each: read stored schema version; if it differs from current, open the
    // capability's NVS namespace and call migrate_from(). Log every migration via
    // the structured log. Called by app_runtime after persistence starts but
    // before other capabilities start.
    void run_migrations(const capability_list& list);

    // ---- app-level settings (fx.settings.*) ----
    // All operations use the app_namespace from config (default "blusys_app").

    blusys_err_t app_get_u32(const char* key, std::uint32_t* out) const;
    blusys_err_t app_set_u32(const char* key, std::uint32_t value);
    blusys_err_t app_get_i32(const char* key, std::int32_t* out) const;
    blusys_err_t app_set_i32(const char* key, std::int32_t value);
    blusys_err_t app_get_str(const char* key, char* out, std::size_t* len) const;
    blusys_err_t app_set_str(const char* key, const char* value);
    blusys_err_t app_get_blob(const char* key, void* out, std::size_t* len) const;
    blusys_err_t app_set_blob(const char* key, const void* value, std::size_t len);
    blusys_err_t app_erase(const char* key);

    struct effects {
        persistence_capability *self = nullptr;

        [[nodiscard]] bool is_ready() const
        {
            return self != nullptr && self->status().nvs_ready;
        }

        blusys_err_t get_u32(const char* key, std::uint32_t* out) const
        {
            return self != nullptr
                ? self->app_get_u32(key, out)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t set_u32(const char* key, std::uint32_t value)
        {
            return self != nullptr
                ? self->app_set_u32(key, value)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t get_i32(const char* key, std::int32_t* out) const
        {
            return self != nullptr
                ? self->app_get_i32(key, out)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t set_i32(const char* key, std::int32_t value)
        {
            return self != nullptr
                ? self->app_set_i32(key, value)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t get_str(const char* key, char* out, std::size_t* len) const
        {
            return self != nullptr
                ? self->app_get_str(key, out, len)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t set_str(const char* key, const char* value)
        {
            return self != nullptr
                ? self->app_set_str(key, value)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t get_blob(const char* key, void* out, std::size_t* len) const
        {
            return self != nullptr
                ? self->app_get_blob(key, out, len)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t set_blob(const char* key, const void* value, std::size_t len)
        {
            return self != nullptr
                ? self->app_set_blob(key, value, len)
                : BLUSYS_ERR_INVALID_STATE;
        }

        blusys_err_t erase(const char* key)
        {
            return self != nullptr
                ? self->app_erase(key)
                : BLUSYS_ERR_INVALID_STATE;
        }

        void bind(persistence_capability *persistence) noexcept
        {
            self = persistence;
        }
    };

private:
    void post_event(persistence_event ev, std::uint32_t code = 0)
    {
        post_integration_event(static_cast<std::uint32_t>(ev), code);
    }

    persistence_config  cfg_;
    persistence_status  status_{};

    struct impl;
    impl* impl_ = nullptr;
};

}  // namespace blusys
