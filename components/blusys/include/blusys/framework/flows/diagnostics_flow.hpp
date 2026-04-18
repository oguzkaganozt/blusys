#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/capabilities/diagnostics.hpp"
#include "lvgl.h"

namespace blusys::flows {

struct diagnostics_display_config {
    bool show_heap   = true;
    bool show_uptime = true;
    bool show_chip   = true;
    bool show_flash  = true;
    bool show_observability = true;
    bool show_crash_dump    = true;
};

struct diagnostics_panel_handles {
    lv_obj_t *heap_kv     = nullptr;
    lv_obj_t *min_heap_kv = nullptr;
    lv_obj_t *uptime_kv   = nullptr;
    lv_obj_t *chip_kv     = nullptr;
    lv_obj_t *flash_kv    = nullptr;
    lv_obj_t *idf_kv      = nullptr;
    lv_obj_t *logs_kv     = nullptr;
    lv_obj_t *counters_kv = nullptr;
    lv_obj_t *error_kv    = nullptr;
    lv_obj_t *crash_kv    = nullptr;
};

// Create a diagnostics info panel as a child of parent.
// Not a standalone screen — embeddable in status/about/diagnostics screens.
// If handles is non-null, widget pointers are stored for later update calls.
lv_obj_t *diagnostics_panel_create(lv_obj_t *parent,
                                    diagnostics_panel_handles *handles = nullptr,
                                    const diagnostics_display_config &config = {});

// Update the panel widgets from the current diagnostics_status.
void diagnostics_panel_update(diagnostics_panel_handles &handles,
                               const diagnostics_status &status);

}  // namespace blusys::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
