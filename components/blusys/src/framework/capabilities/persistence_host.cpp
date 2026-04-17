#include "blusys/framework/capabilities/persistence.hpp"
#include "blusys/framework/capabilities/list.hpp"
#include "blusys/framework/observe/log.h"

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace blusys {

// In-memory key-value store for host builds. Keyed by (namespace, key).
// Values are stored as byte vectors (serialized representation).

using kv_key_t   = std::pair<std::string, std::string>;
using kv_value_t = std::vector<std::uint8_t>;
using kv_store_t = std::map<kv_key_t, kv_value_t>;

struct persistence_capability::impl {
    kv_store_t store;
    std::string app_ns;

    void put(const std::string& ns, const std::string& key, const void* data,
             std::size_t len)
    {
        auto& v = store[{ns, key}];
        v.assign(static_cast<const std::uint8_t*>(data),
                 static_cast<const std::uint8_t*>(data) + len);
    }

    bool get(const std::string& ns, const std::string& key, void* out,
             std::size_t* len) const
    {
        auto it = store.find({ns, key});
        if (it == store.end()) {
            return false;
        }
        if (out != nullptr && *len >= it->second.size()) {
            std::memcpy(out, it->second.data(), it->second.size());
        }
        *len = it->second.size();
        return true;
    }

    bool erase(const std::string& ns, const std::string& key)
    {
        return store.erase({ns, key}) > 0;
    }
};

persistence_capability::persistence_capability(const persistence_config& cfg)
    : cfg_(cfg), impl_(new impl{})
{
    impl_->app_ns = cfg_.app_namespace != nullptr ? cfg_.app_namespace : "blusys_app";
}

persistence_capability::~persistence_capability()
{
    delete impl_;
}

blusys_err_t persistence_capability::start(runtime& rt)
{
    rt_ = &rt;
    status_.nvs_ready      = true;
    status_.capability_ready = true;
    post_event(persistence_event::ready);
    BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_persistence,
               "host stub: persistence ready (in-memory), app namespace: %s",
               impl_->app_ns.c_str());
    return BLUSYS_OK;
}

void persistence_capability::poll(std::uint32_t /*now_ms*/) {}

void persistence_capability::stop()
{
    status_ = {};
    rt_     = nullptr;
}

void persistence_capability::run_migrations(const capability_list& list)
{
    // On host: capabilities with persistence schemas are discovered and
    // version-checked against the in-memory meta store. migrate_from() is
    // called when versions differ. In practice, the in-memory store starts
    // empty (no stored versions), so first-run initialization is logged but
    // no data migration is required.
    static constexpr const char* kMetaNs = "blusys.meta";

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
        std::size_t    vlen    = sizeof(stored);

        bool found = impl_->get(kMetaNs, ns, &stored, &vlen);
        if (!found) {
            impl_->put(kMetaNs, ns, &current, sizeof(current));
            BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_persistence,
                       "schema v%u registered: %s", current, ns);
            continue;
        }

        if (stored == current) {
            continue;
        }

        BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_persistence,
                   "migrating %s: v%u → v%u (host: no-op)", ns, stored, current);

        // Host has no real NVS handle; pass nullptr. Capabilities that
        // override migrate_from() must handle a nullptr nvs on host.
        blusys_err_t err = schema->migrate_from(stored, nullptr);
        if (err != BLUSYS_OK) {
            BLUSYS_LOG(BLUSYS_LOG_ERROR, err_domain_persistence,
                       "migration failed for %s: %d", ns,
                       static_cast<int>(err));
            post_event(persistence_event::migration_failed,
                       static_cast<std::uint32_t>(err));
        } else {
            impl_->put(kMetaNs, ns, &current, sizeof(current));
            ++status_.migrations_run;
            post_event(persistence_event::migration_done);
            BLUSYS_LOG(BLUSYS_LOG_INFO, err_domain_persistence,
                       "migration done: %s v%u", ns, current);
        }
    }
}

// ---- app settings (in-memory on host) ----

blusys_err_t persistence_capability::app_get_u32(const char* key,
                                                   std::uint32_t* out) const
{
    if (key == nullptr || out == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    std::size_t len = sizeof(*out);
    if (!impl_->get(impl_->app_ns, key, out, &len)) {
        return BLUSYS_ERR_IO;
    }
    return BLUSYS_OK;
}

blusys_err_t persistence_capability::app_set_u32(const char* key,
                                                   std::uint32_t value)
{
    if (key == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    impl_->put(impl_->app_ns, key, &value, sizeof(value));
    return BLUSYS_OK;
}

blusys_err_t persistence_capability::app_get_i32(const char* key,
                                                   std::int32_t* out) const
{
    if (key == nullptr || out == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    std::size_t len = sizeof(*out);
    if (!impl_->get(impl_->app_ns, key, out, &len)) {
        return BLUSYS_ERR_IO;
    }
    return BLUSYS_OK;
}

blusys_err_t persistence_capability::app_set_i32(const char* key,
                                                   std::int32_t value)
{
    if (key == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    impl_->put(impl_->app_ns, key, &value, sizeof(value));
    return BLUSYS_OK;
}

blusys_err_t persistence_capability::app_get_str(const char* key, char* out,
                                                   std::size_t* len) const
{
    if (key == nullptr || len == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    std::size_t needed = 0;
    auto it = impl_->store.find({impl_->app_ns, key});
    if (it == impl_->store.end()) {
        return BLUSYS_ERR_IO;
    }
    needed = it->second.size();
    if (out != nullptr && *len >= needed) {
        std::memcpy(out, it->second.data(), needed);
    }
    *len = needed;
    return BLUSYS_OK;
}

blusys_err_t persistence_capability::app_set_str(const char* key,
                                                   const char* value)
{
    if (key == nullptr || value == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    std::size_t len = std::strlen(value) + 1;
    impl_->put(impl_->app_ns, key, value, len);
    return BLUSYS_OK;
}

blusys_err_t persistence_capability::app_get_blob(const char* key, void* out,
                                                    std::size_t* len) const
{
    if (key == nullptr || len == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (!impl_->get(impl_->app_ns, key, out, len)) {
        return BLUSYS_ERR_IO;
    }
    return BLUSYS_OK;
}

blusys_err_t persistence_capability::app_set_blob(const char* key,
                                                    const void* value,
                                                    std::size_t len)
{
    if (key == nullptr || value == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    impl_->put(impl_->app_ns, key, value, len);
    return BLUSYS_OK;
}

blusys_err_t persistence_capability::app_erase(const char* key)
{
    if (key == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    impl_->erase(impl_->app_ns, key);
    return BLUSYS_OK;
}

}  // namespace blusys
