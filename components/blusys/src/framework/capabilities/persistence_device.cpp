#include "blusys/framework/capabilities/persistence.hpp"
#include "blusys/framework/capabilities/list.hpp"
#include "blusys/framework/observe/log.h"
#include "blusys/hal/nvs.h"
#include "blusys/hal/internal/nvs_init.h"

#include <cstring>

// NVS namespace for storing per-capability schema versions.
static constexpr const char* kMetaNs = "blusys.meta";

namespace blusys {

struct persistence_capability::impl {
    blusys_nvs_t* app_nvs = nullptr;  // open handle for app-level settings
};

persistence_capability::persistence_capability(const persistence_config& cfg)
    : cfg_(cfg), impl_(new impl{})
{
}

persistence_capability::~persistence_capability()
{
    delete impl_;
}

blusys_err_t persistence_capability::start(runtime& rt)
{
    rt_ = &rt;

    blusys_err_t err = blusys_nvs_ensure_init();
    if (err != BLUSYS_OK) {
        BLUSYS_LOG(BLUSYS_LOG_ERROR, err_domain_persistence,
                   "NVS flash init failed: %d", static_cast<int>(err));
        return err;
    }

    const char* ns = cfg_.app_namespace != nullptr ? cfg_.app_namespace : "blusys_app";
    err = blusys_nvs_open(ns, BLUSYS_NVS_READWRITE, &impl_->app_nvs);
    if (err != BLUSYS_OK) {
        BLUSYS_LOG(BLUSYS_LOG_ERROR, err_domain_persistence,
                   "app namespace open failed (%s): %d", ns, static_cast<int>(err));
        return err;
    }

    status_.nvs_ready = true;
    status_.capability_ready = true;
    post_event(persistence_event::ready);
    BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_persistence,
               "NVS ready, app namespace: %s", ns);
    return BLUSYS_OK;
}

void persistence_capability::poll(std::uint32_t /*now_ms*/)
{
    // NVS operations are synchronous; nothing to poll.
}

void persistence_capability::stop()
{
    if (impl_->app_nvs != nullptr) {
        blusys_nvs_close(impl_->app_nvs);
        impl_->app_nvs = nullptr;
    }
    status_ = {};
    rt_ = nullptr;
}

void persistence_capability::run_migrations(const capability_list& list)
{
    if (!status_.nvs_ready) {
        return;
    }

    blusys_nvs_t* meta = nullptr;
    blusys_err_t err = blusys_nvs_open(kMetaNs, BLUSYS_NVS_READWRITE, &meta);
    if (err != BLUSYS_OK) {
        BLUSYS_LOG(BLUSYS_LOG_WARN, err_domain_persistence,
                   "meta namespace open failed: %d — migrations skipped",
                   static_cast<int>(err));
        return;
    }

    for (std::size_t i = 0; i < list.count; ++i) {
        capability_base* cap = list.items[i];
        if (cap == nullptr) {
            continue;
        }

        capability_persistence_schema* schema = cap->as_persistence_schema();
        if (schema == nullptr) {
            continue;
        }

        const char*    ns      = schema->persistence_namespace();
        std::uint32_t  current = schema->schema_version();
        std::uint32_t  stored  = 0;

        blusys_err_t verr = blusys_nvs_get_u32(meta, ns, &stored);
        if (verr == BLUSYS_ERR_IO) {
            // Key not found: first boot for this capability. Write current version.
            blusys_nvs_set_u32(meta, ns, current);
            blusys_nvs_commit(meta);
            BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_persistence,
                       "schema v%lu registered: %s",
                       static_cast<unsigned long>(current), ns);
            continue;
        }
        if (verr != BLUSYS_OK) {
            BLUSYS_LOG(BLUSYS_LOG_WARN, err_domain_persistence,
                       "version read failed for %s: %d — skipping", ns,
                       static_cast<int>(verr));
            continue;
        }

        if (stored == current) {
            continue;  // up to date
        }

        BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_persistence,
                   "migrating %s: v%lu → v%lu", ns,
                   static_cast<unsigned long>(stored),
                   static_cast<unsigned long>(current));

        blusys_nvs_t* cap_nvs = nullptr;
        err = blusys_nvs_open(ns, BLUSYS_NVS_READWRITE, &cap_nvs);
        if (err != BLUSYS_OK) {
            BLUSYS_LOG(BLUSYS_LOG_ERROR, err_domain_persistence,
                       "migration open failed (%s): %d", ns,
                       static_cast<int>(err));
            post_event(persistence_event::migration_failed,
                       static_cast<std::uint32_t>(err));
            continue;
        }

        blusys_err_t merr = schema->migrate_from(stored, cap_nvs);
        blusys_nvs_close(cap_nvs);

        if (merr != BLUSYS_OK) {
            BLUSYS_LOG(BLUSYS_LOG_ERROR, err_domain_persistence,
                       "migration failed for %s (v%lu→v%lu): %d", ns,
                       static_cast<unsigned long>(stored),
                       static_cast<unsigned long>(current),
                       static_cast<int>(merr));
            post_event(persistence_event::migration_failed,
                       static_cast<std::uint32_t>(merr));
        } else {
            blusys_nvs_set_u32(meta, ns, current);
            blusys_nvs_commit(meta);
            ++status_.migrations_run;
            post_event(persistence_event::migration_done);
            BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_persistence,
                       "migration done: %s v%lu", ns,
                       static_cast<unsigned long>(current));
        }
    }

    blusys_nvs_close(meta);
}

// ---- app settings ----

blusys_err_t persistence_capability::app_get_u32(const char* key,
                                                   std::uint32_t* out) const
{
    if (impl_->app_nvs == nullptr || key == nullptr || out == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return blusys_nvs_get_u32(impl_->app_nvs, key, out);
}

blusys_err_t persistence_capability::app_set_u32(const char* key,
                                                   std::uint32_t value)
{
    if (impl_->app_nvs == nullptr || key == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    blusys_err_t err = blusys_nvs_set_u32(impl_->app_nvs, key, value);
    if (err == BLUSYS_OK) {
        blusys_nvs_commit(impl_->app_nvs);
    }
    return err;
}

blusys_err_t persistence_capability::app_get_i32(const char* key,
                                                   std::int32_t* out) const
{
    if (impl_->app_nvs == nullptr || key == nullptr || out == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return blusys_nvs_get_i32(impl_->app_nvs, key, out);
}

blusys_err_t persistence_capability::app_set_i32(const char* key,
                                                   std::int32_t value)
{
    if (impl_->app_nvs == nullptr || key == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    blusys_err_t err = blusys_nvs_set_i32(impl_->app_nvs, key, value);
    if (err == BLUSYS_OK) {
        blusys_nvs_commit(impl_->app_nvs);
    }
    return err;
}

blusys_err_t persistence_capability::app_get_str(const char* key, char* out,
                                                   std::size_t* len) const
{
    if (impl_->app_nvs == nullptr || key == nullptr || len == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return blusys_nvs_get_str(impl_->app_nvs, key, out, len);
}

blusys_err_t persistence_capability::app_set_str(const char* key,
                                                   const char* value)
{
    if (impl_->app_nvs == nullptr || key == nullptr || value == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    blusys_err_t err = blusys_nvs_set_str(impl_->app_nvs, key, value);
    if (err == BLUSYS_OK) {
        blusys_nvs_commit(impl_->app_nvs);
    }
    return err;
}

blusys_err_t persistence_capability::app_get_blob(const char* key, void* out,
                                                    std::size_t* len) const
{
    if (impl_->app_nvs == nullptr || key == nullptr || len == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return blusys_nvs_get_blob(impl_->app_nvs, key, out, len);
}

blusys_err_t persistence_capability::app_set_blob(const char* key,
                                                    const void* value,
                                                    std::size_t len)
{
    if (impl_->app_nvs == nullptr || key == nullptr || value == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    blusys_err_t err = blusys_nvs_set_blob(impl_->app_nvs, key, value, len);
    if (err == BLUSYS_OK) {
        blusys_nvs_commit(impl_->app_nvs);
    }
    return err;
}

blusys_err_t persistence_capability::app_erase(const char* key)
{
    if (impl_->app_nvs == nullptr || key == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    blusys_err_t err = blusys_nvs_erase_key(impl_->app_nvs, key);
    if (err == BLUSYS_OK) {
        blusys_nvs_commit(impl_->app_nvs);
    }
    return err;
}

}  // namespace blusys
