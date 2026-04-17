#pragma once

#include <cstdint>
#include <optional>
#include "blusys/blusys.hpp"


namespace atlas_example {

// Operational state machine — mirrors the Atlas backend's understanding of
// a device's lifecycle. Reducer-driven and re-computed from flags after
// every action.
enum class op_state : std::uint8_t {
    idle,
    connecting_wifi,
    time_syncing,
    connecting_broker,
    online,
    updating,
    error,
};

const char *op_state_name(op_state s);

// Reserved event IDs posted by atlas_capability. Must live inside the
// product-custom integration band (0x0900–0x09FF — see
// blusys/framework/capabilities/capability.hpp).
//
// broker lifecycle comes from the underlying mqtt_capability (canonical
// `capability_event_tag::mqtt_{connected,disconnected,error}`) — the
// Atlas adapter only re-emits _connected/_disconnected as Atlas-named
// events so downstream products don't have to know transport specifics.
enum class atlas_event : std::uint32_t {
    broker_connected    = 0x0901,
    broker_disconnected = 0x0902,
    command_received    = 0x0910,
    state_desired       = 0x0911,
    ota_announcement    = 0x0912,
};

// Inbound MQTT payloads now travel as `blusys::mqtt_message*` — see
// `integration_passthrough` events from atlas_capability. Storage is
// owned by `mqtt_capability::drained_` (persists across one reducer pass).

enum class action_tag : std::uint8_t {
    capability_event,
    sample_tick,
};

struct action {
    action_tag              tag = action_tag::sample_tick;
    blusys::capability_event cap_event{};
};

// Reducer state — persisted across ticks. Single source of truth for
// the phase machine. Transient telemetry (free_heap, rssi) is NOT cached
// here; on_tick reads it from ctx / HAL at publish time.
struct app_state {
    op_state      phase             = op_state::idle;

    // Driven by capability events in `update()`.
    bool          wifi_connected    = false;
    bool          has_ip            = false;
    bool          time_synced       = false;
    bool          broker_connected  = false;
    bool          ota_in_progress   = false;
    std::uint8_t  ota_progress_pct  = 0;

    // Monotonic counters visible in state payloads.
    std::uint32_t commands_handled  = 0;
    std::uint32_t heartbeats_sent   = 0;
    std::uint32_t states_published  = 0;

    // Publish scheduling (on_tick only).
    std::uint32_t last_heartbeat_ms = 0;
    std::uint32_t last_state_ms     = 0;
};

void update(blusys::app_ctx &ctx, app_state &state, const action &event);

// Unified event hook used by atlas_capability → reducer.
std::optional<action> on_event(blusys::event event, app_state &state);

}  // namespace atlas_example
