#pragma once

#include <cstdint>

#include "blusys/app/app.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "lvgl.h"
#endif

namespace edge_node {

// ---- operational state machine ----

enum class op_state : std::uint8_t {
    idle,
    provisioning,
    connecting,
    connected,
    reporting,
    updating,
    error,
};

// ---- actions ----

enum class action_tag : std::uint8_t {
    // connectivity
    wifi_started,
    wifi_connected,
    wifi_connecting,
    wifi_got_ip,
    wifi_disconnected,
    wifi_reconnecting,
    time_synced,
    time_sync_failed,
    mdns_ready,
    local_ctrl_ready,
    conn_capability_ready,

    // telemetry
    telemetry_delivered,
    telemetry_failed,
    telemetry_buffer_full,
    telemetry_capability_ready,
    telemetry_buffer_flushed,

    // diagnostics
    diag_snapshot,
    diag_capability_ready,

    // ota
    ota_download_started,
    ota_download_progress,
    ota_download_complete,
    ota_download_failed,
    ota_apply_complete,
    ota_apply_failed,
    ota_rollback_pending,
    ota_marked_valid,
    ota_capability_ready,

    // provisioning
    prov_started,
    prov_credentials_received,
    prov_success,
    prov_failed,
    prov_already_done,
    prov_reset_complete,
    prov_capability_ready,

    // storage
    storage_spiffs_mounted,
    storage_fatfs_mounted,
    storage_capability_ready,

    // periodic
    sample_tick,
};

struct action {
    action_tag    tag;
    std::int32_t  value = 0;
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

    // simulated sensor
    float         temperature     = 22.5f;
    float         humidity        = 48.0f;

    // ota
    std::uint8_t  ota_progress    = 0;
    bool          ota_in_progress = false;

    // diagnostics cache
    std::uint32_t free_heap       = 0;
    std::uint32_t uptime_ms       = 0;

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    // Optional SSD1306 mono UI — filled in ui/on_init + screen factory; timer reads these.
    struct local_mono_ui {
        lv_obj_t *phase_lbl = nullptr;
        lv_obj_t *temp_lbl  = nullptr;
        lv_obj_t *hum_lbl   = nullptr;
        lv_obj_t *tel_lbl   = nullptr;
    } mono_ui{};
#endif
};

// ---- public functions ----

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);

const char *op_state_name(op_state s);

}  // namespace edge_node
