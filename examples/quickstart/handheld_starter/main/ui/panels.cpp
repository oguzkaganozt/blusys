#include "ui/panels.hpp"

#include "core/app_logic.hpp"

#include "blusys/framework/flows/provisioning_flow.hpp"

#include <cstdio>

namespace handheld_starter::ui {

void ShellChrome::sync(const app_state &state, const char *accent_line)
{
    if (badge != nullptr) {
        const bool setup_ready = state.provisioning.capability_ready || state.provisioning.is_provisioned;
        blusys::app::view::set_badge_text(badge, setup_ready ? "Ready" : "Setup");
        blusys::app::view::set_badge_level(
            badge,
            setup_ready ? blusys::ui::badge_level::success : blusys::ui::badge_level::warning);
    }

    if (detail != nullptr) {
        char line[48];
        std::snprintf(line, sizeof(line), "%s  %ld%%", accent_line,
                      static_cast<long>(state.level));
        blusys::app::view::set_text(detail, line);
    }
}

void HomePanel::sync(const app_state &state)
{
    blusys::app::view::sync_percent_output(output, state.level);

    if (preset_kv != nullptr) {
        blusys::app::view::set_kv_value(preset_kv, accent_name(state.accent_index));
    }
    if (hold_badge != nullptr) {
        blusys::app::view::set_badge_text(hold_badge,
                                          state.hold_enabled ? "Hold" : "Live");
        blusys::app::view::set_badge_level(
            hold_badge,
            state.hold_enabled ? blusys::ui::badge_level::warning
                               : blusys::ui::badge_level::success);
    }
}

void StatusPanel::sync(const app_state &state)
{
    if (setup_badge != nullptr) {
        const bool ready = state.provisioning.capability_ready || state.provisioning.is_provisioned;
        blusys::app::view::set_badge_text(setup_badge, ready ? "Provisioned" : "Waiting");
        blusys::app::view::set_badge_level(
            setup_badge,
            ready ? blusys::ui::badge_level::success : blusys::ui::badge_level::warning);
    }
    if (storage_kv != nullptr) {
        blusys::app::view::set_kv_value(storage_kv,
                                        state.storage_ready ? "Mounted" : "Pending");
    }
    if (output_kv != nullptr) {
        char value[16];
        std::snprintf(value, sizeof(value), "%ld%%", static_cast<long>(state.level));
        blusys::app::view::set_kv_value(output_kv, value);
    }
    if (hold_kv != nullptr) {
        blusys::app::view::set_kv_value(hold_kv,
                                        state.hold_enabled ? "Enabled" : "Off");
    }
}

void sync_all_panels(app_state &state)
{
    state.shell.sync(state, accent_name(state.accent_index));
    state.home.sync(state);
    state.status.sync(state);
    blusys::app::flows::provisioning_screen_update(state.setup_handles, state.provisioning);
}

}  // namespace handheld_starter::ui
