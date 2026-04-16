#pragma once

#include "blusys/framework/capabilities/capability.hpp"
#include "blusys/framework/capabilities/mqtt.hpp"
#include "blusys/framework/capabilities/ota.hpp"

#include "core/atlas_logic.hpp"

#include <cstddef>
#include <cstdint>

namespace atlas_example {

// Identity and firmware metadata. Auth + broker transport live in the
// shared `mqtt_capability` the owner composes alongside this one.
struct atlas_config {
    const char *device_id        = nullptr;
    const char *firmware_version = "1.0.0";
};

// Atlas-protocol adapter. Stays in product code because it encodes the
// Atlas wire conventions — topic layout, retained state shape, ota
// announcement schema — not a reusable framework primitive.
//
// Composition contract:
//   - `mqtt` must be pre-populated with per-device auth, CA bundle, and
//     broker URL (atlas_main.cpp wires that). `atlas_capability` adds its
//     subscriptions at runtime once MQTT is connected, and installs itself
//     as `mqtt`'s direct message handler.
//   - `ota` is invoked via `request_update(url)` when an Atlas OTA
//     announcement arrives — the URL is pulled from the announcement JSON
//     rather than baked into `ota_config`.
class atlas_capability final : public blusys::capability_base {
public:
    atlas_capability(const atlas_config &cfg,
                     blusys::mqtt_capability *mqtt,
                     blusys::ota_capability  *ota);

    atlas_capability(const atlas_capability &)            = delete;
    atlas_capability &operator=(const atlas_capability &) = delete;

    [[nodiscard]] blusys::capability_kind kind() const override
    {
        return blusys::capability_kind::custom;
    }

    blusys_err_t start(blusys::runtime &rt) override;
    void         poll(std::uint32_t now_ms) override;
    void         stop() override;

    // App-thread publish helpers (reducer / on_tick).
    blusys_err_t publish_state(const char *json, std::size_t len);
    blusys_err_t publish_heartbeat(const char *json, std::size_t len);
    blusys_err_t publish_event(const char *json, std::size_t len);

private:
    static void on_mqtt_message(const blusys::mqtt_message &msg, void *user_ctx);
    void        handle_mqtt_message(const blusys::mqtt_message &msg);
    void        on_mqtt_connected();

    atlas_config              cfg_;
    blusys::mqtt_capability  *mqtt_ = nullptr;
    blusys::ota_capability   *ota_  = nullptr;

    bool was_connected_       = false;

    char topic_up_state_[128]     = {};
    char topic_up_heartbeat_[128] = {};
    char topic_up_event_[128]     = {};
    char topic_down_command_[128] = {};
    char topic_down_state_[128]   = {};
    char topic_down_ota_[128]     = {};
};

}  // namespace atlas_example
