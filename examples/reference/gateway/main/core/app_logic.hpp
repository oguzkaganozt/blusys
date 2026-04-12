#pragma once

#include <cstdint>

#include "blusys/app/app.hpp"
#include "blusys/app/capability_event.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/screens/status_screen.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "lvgl.h"
#endif

namespace gateway {

// ---- operational state machine ----

enum class op_state : std::uint8_t {
    idle,
    provisioning,
    connecting,
    connected,
    operational,
    updating,
    error,
};

// ---- routes (interactive variant) ----

enum route_id : std::uint32_t {
    route_dashboard = 1,
    route_status,
    route_settings,
    route_about,
};

// ---- actions ----

enum class action_tag : std::uint8_t {
    capability_event,

    // navigation (interactive)
    show_dashboard,
    show_status,
    show_settings,
    open_about,

    // periodic
    sample_tick,
};

struct action {
    action_tag                      tag = action_tag::sample_tick;
    std::int32_t                    value = 0;
    blusys::app::capability_event   cap_event{};
};

// ---- app state ----

struct app_state {
    op_state      phase           = op_state::idle;
    bool          wifi_connected  = false;
    bool          has_ip          = false;
    bool          time_synced     = false;
    bool          conn_ready      = false;
    bool          storage_ready   = false;
    bool          provisioned     = false;
    bool          diag_ready      = false;
    bool          ota_ready       = false;

    // telemetry
    std::uint32_t sample_count    = 0;
    std::uint16_t delivered       = 0;
    std::uint16_t delivery_fails  = 0;

    // simulated downstream devices
    std::int32_t  device_count    = 3;
    std::int32_t  active_devices  = 0;
    float         agg_throughput  = 0.0f;

    // ota
    std::uint8_t  ota_progress    = 0;
    bool          ota_in_progress = false;

    // diagnostics cache
    blusys::app::diagnostics_status diagnostics{};

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    // widget handles (interactive variant)
    lv_obj_t *shell_badge     = nullptr;
    lv_obj_t *shell_detail    = nullptr;
    lv_obj_t *dash_devices    = nullptr;
    lv_obj_t *dash_throughput = nullptr;
    lv_obj_t *dash_uplink     = nullptr;
    lv_obj_t *dash_storage    = nullptr;
    lv_obj_t *dash_table      = nullptr;

    blusys::app::screens::status_screen_handles status_handles{};
#endif
};

// ---- public functions ----

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);
bool map_intent(blusys::app::app_services &svc, blusys::framework::intent intent, action *out);

const char *op_state_name(op_state s);

}  // namespace gateway
