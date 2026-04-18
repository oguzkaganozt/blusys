#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/flows/diagnostics_flow.hpp"
#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"

#include <cstdio>

namespace blusys::flows {

lv_obj_t *diagnostics_panel_create(lv_obj_t *parent,
                                    diagnostics_panel_handles *handles,
                                    const diagnostics_display_config &config)
{
    const auto &t = blusys::theme();

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(panel, t.spacing_xs, 0);

    if (config.show_heap) {
        lv_obj_t *heap = blusys::key_value_create(panel, {.key = "Free Heap", .value = "\xe2\x80\x94"});
        lv_obj_t *min_heap = blusys::key_value_create(panel, {.key = "Min Heap", .value = "\xe2\x80\x94"});
        if (handles != nullptr) {
            handles->heap_kv = heap;
            handles->min_heap_kv = min_heap;
        }
    }

    if (config.show_uptime) {
        lv_obj_t *uptime = blusys::key_value_create(panel, {.key = "Uptime", .value = "\xe2\x80\x94"});
        if (handles != nullptr) { handles->uptime_kv = uptime; }
    }

    if (config.show_chip) {
        lv_obj_t *chip = blusys::key_value_create(panel, {.key = "Chip", .value = "\xe2\x80\x94"});
        lv_obj_t *idf = blusys::key_value_create(panel, {.key = "IDF", .value = "\xe2\x80\x94"});
        if (handles != nullptr) {
            handles->chip_kv = chip;
            handles->idf_kv = idf;
        }
    }

    if (config.show_flash) {
        lv_obj_t *flash = blusys::key_value_create(panel, {.key = "Flash", .value = "\xe2\x80\x94"});
        if (handles != nullptr) { handles->flash_kv = flash; }
    }

    if (config.show_observability) {
        lv_obj_t *logs = blusys::key_value_create(panel, {.key = "Log ring", .value = "\xe2\x80\x94"});
        lv_obj_t *counters = blusys::key_value_create(panel, {.key = "Counters", .value = "\xe2\x80\x94"});
        lv_obj_t *last_error = blusys::key_value_create(panel, {.key = "Last error", .value = "\xe2\x80\x94"});
        if (handles != nullptr) {
            handles->logs_kv = logs;
            handles->counters_kv = counters;
            handles->error_kv = last_error;
        }
    }

    if (config.show_crash_dump) {
        lv_obj_t *crash = blusys::key_value_create(panel, {.key = "Crash dump", .value = "\xe2\x80\x94"});
        if (handles != nullptr) { handles->crash_kv = crash; }
    }

    return panel;
}

void diagnostics_panel_update(diagnostics_panel_handles &handles,
                               const diagnostics_status &status)
{
    char buf[256];
    const diagnostics_snapshot &snapshot = status.last_snapshot;

    if (handles.heap_kv != nullptr) {
        std::snprintf(buf, sizeof(buf), "%lu B",
                      static_cast<unsigned long>(snapshot.free_heap));
        blusys::key_value_set_value(handles.heap_kv, buf);
    }

    if (handles.min_heap_kv != nullptr) {
        std::snprintf(buf, sizeof(buf), "%lu B",
                      static_cast<unsigned long>(snapshot.min_free_heap));
        blusys::key_value_set_value(handles.min_heap_kv, buf);
    }

    if (handles.uptime_kv != nullptr) {
        std::uint32_t secs = snapshot.uptime_ms / 1000;
        std::uint32_t mins = secs / 60;
        std::uint32_t hrs  = mins / 60;
        std::snprintf(buf, sizeof(buf), "%lu:%02lu:%02lu",
                      static_cast<unsigned long>(hrs),
                      static_cast<unsigned long>(mins % 60),
                      static_cast<unsigned long>(secs % 60));
        blusys::key_value_set_value(handles.uptime_kv, buf);
    }

    if (handles.chip_kv != nullptr) {
        std::snprintf(buf, sizeof(buf), "%s (%uc)",
                      snapshot.chip_model,
                      static_cast<unsigned>(snapshot.chip_cores));
        blusys::key_value_set_value(handles.chip_kv, buf);
    }

    if (handles.idf_kv != nullptr) {
        blusys::key_value_set_value(handles.idf_kv, snapshot.idf_version);
    }

    if (handles.flash_kv != nullptr) {
        std::uint32_t mb = snapshot.flash_size / (1024 * 1024);
        if (mb > 0) {
            std::snprintf(buf, sizeof(buf), "%lu MB",
                          static_cast<unsigned long>(mb));
        } else {
            std::snprintf(buf, sizeof(buf), "%lu KB",
                          static_cast<unsigned long>(snapshot.flash_size / 1024));
        }
        blusys::key_value_set_value(handles.flash_kv, buf);
    }

    if (handles.logs_kv != nullptr) {
        std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(status.observability.log_count));
        blusys::key_value_set_value(handles.logs_kv, buf);
    }

    if (handles.counters_kv != nullptr) {
        std::uint64_t total = 0;
        for (int i = 0; i < (int)err_domain_count; ++i) {
            total += status.observability.counters.values[i];
        }
        std::snprintf(buf, sizeof(buf), "%llu total",
                      static_cast<unsigned long long>(total));
        blusys::key_value_set_value(handles.counters_kv, buf);
    }

    if (handles.error_kv != nullptr) {
        const blusys_observe_last_error_t *best = nullptr;
        for (int i = 0; i < (int)err_domain_count; ++i) {
            const auto &candidate = status.observability.last_errors[i];
            if (candidate.valid && (best == nullptr || candidate.sequence > best->sequence)) {
                best = &candidate;
            }
        }
        if (best != nullptr) {
            std::snprintf(buf, sizeof(buf), "%s: %s",
                          blusys_err_domain_string(best->domain), best->text);
        } else {
            std::snprintf(buf, sizeof(buf), "none");
        }
        blusys::key_value_set_value(handles.error_kv, buf);
    }

    if (handles.crash_kv != nullptr) {
        if (status.crash_dump_ready) {
            std::snprintf(buf, sizeof(buf), "%u log lines",
                          static_cast<unsigned>(status.crash_dump.log_count));
        } else {
            std::snprintf(buf, sizeof(buf), "none");
        }
        blusys::key_value_set_value(handles.crash_kv, buf);
    }
}

}  // namespace blusys::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
