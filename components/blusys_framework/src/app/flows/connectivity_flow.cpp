#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/flows/connectivity_flow.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/primitives/status_badge.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/framework/ui/primitives/label.hpp"
#include "blusys/framework/ui/primitives/divider.hpp"

namespace blusys::app::flows {

lv_obj_t *connectivity_panel_create(lv_obj_t *parent,
                                     connectivity_panel_handles *handles,
                                     const connectivity_display_config &config)
{
    const auto &t = blusys::ui::theme();

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(panel, t.spacing_xs, 0);

    // Wi-Fi status badge.
    lv_obj_t *wifi = blusys::ui::status_badge_create(panel, {
        .text = "WiFi",
        .level = blusys::ui::badge_level::info,
    });
    if (handles != nullptr) { handles->wifi_badge = wifi; }

    // IP address.
    if (config.show_ip) {
        lv_obj_t *ip = blusys::ui::key_value_create(panel, {
            .key = "IP",
            .value = "\xe2\x80\x94",  // em-dash
        });
        if (handles != nullptr) { handles->ip_label = ip; }
    }

    // Service badges.
    if (config.show_services) {
        lv_obj_t *sntp = blusys::ui::status_badge_create(panel, {
            .text = "SNTP",
            .level = blusys::ui::badge_level::info,
        });
        if (handles != nullptr) { handles->sntp_badge = sntp; }

        lv_obj_t *mdns = blusys::ui::status_badge_create(panel, {
            .text = "mDNS",
            .level = blusys::ui::badge_level::info,
        });
        if (handles != nullptr) { handles->mdns_badge = mdns; }

        lv_obj_t *ctrl = blusys::ui::status_badge_create(panel, {
            .text = "Ctrl",
            .level = blusys::ui::badge_level::info,
        });
        if (handles != nullptr) { handles->ctrl_badge = ctrl; }
    }

    return panel;
}

void connectivity_panel_update(connectivity_panel_handles &handles,
                                const connectivity_status &status)
{
    if (handles.wifi_badge != nullptr) {
        blusys::ui::status_badge_set_level(handles.wifi_badge,
            status.wifi_connected ? blusys::ui::badge_level::success
                                  : blusys::ui::badge_level::error);
        blusys::ui::status_badge_set_text(handles.wifi_badge,
            status.wifi_connected ? "WiFi: Connected" : "WiFi: Disconnected");
    }

    if (handles.ip_label != nullptr && status.has_ip) {
        blusys::ui::key_value_set_value(handles.ip_label, status.ip_info.ip);
    }

    if (handles.sntp_badge != nullptr) {
        blusys::ui::status_badge_set_level(handles.sntp_badge,
            status.time_synced ? blusys::ui::badge_level::success
                               : blusys::ui::badge_level::warning);
    }

    if (handles.mdns_badge != nullptr) {
        blusys::ui::status_badge_set_level(handles.mdns_badge,
            status.mdns_running ? blusys::ui::badge_level::success
                                : blusys::ui::badge_level::warning);
    }

    if (handles.ctrl_badge != nullptr) {
        blusys::ui::status_badge_set_level(handles.ctrl_badge,
            status.local_ctrl_running ? blusys::ui::badge_level::success
                                      : blusys::ui::badge_level::warning);
    }
}

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
