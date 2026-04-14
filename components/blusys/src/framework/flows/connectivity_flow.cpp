#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/flows/connectivity_flow.hpp"
#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/framework/ui/primitives/status_badge.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/framework/ui/primitives/label.hpp"
#include "blusys/framework/ui/primitives/divider.hpp"
#include "blusys/framework/ui/widgets/button.hpp"

namespace blusys::flows {

lv_obj_t *connectivity_panel_create(lv_obj_t *parent,
                                     connectivity_panel_handles *handles,
                                     const connectivity_display_config &config)
{
    const auto &t = blusys::theme();

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(panel, t.spacing_xs, 0);

    // Wi-Fi status badge.
    lv_obj_t *wifi = blusys::status_badge_create(panel, {
        .text = "WiFi",
        .level = blusys::badge_level::info,
    });
    if (handles != nullptr) { handles->wifi_badge = wifi; }

    // IP address.
    if (config.show_ip) {
        lv_obj_t *ip = blusys::key_value_create(panel, {
            .key = "IP",
            .value = "\xe2\x80\x94",  // em-dash
        });
        if (handles != nullptr) { handles->ip_label = ip; }
    }

    // Service badges.
    if (config.show_services) {
        lv_obj_t *sntp = blusys::status_badge_create(panel, {
            .text = "SNTP",
            .level = blusys::badge_level::info,
        });
        if (handles != nullptr) { handles->sntp_badge = sntp; }

        lv_obj_t *mdns = blusys::status_badge_create(panel, {
            .text = "mDNS",
            .level = blusys::badge_level::info,
        });
        if (handles != nullptr) { handles->mdns_badge = mdns; }

        lv_obj_t *ctrl = blusys::status_badge_create(panel, {
            .text = "Ctrl",
            .level = blusys::badge_level::info,
        });
        if (handles != nullptr) { handles->ctrl_badge = ctrl; }
    }

    return panel;
}

void connectivity_panel_update(connectivity_panel_handles &handles,
                                const connectivity_status &status)
{
    if (handles.wifi_badge != nullptr) {
        blusys::badge_level lvl = blusys::badge_level::error;
        const char *txt = "WiFi: Disconnected";
        if (status.wifi_reconnecting) {
            lvl = blusys::badge_level::warning;
            txt = "WiFi: Reconnecting…";
        } else if (status.wifi_connecting && !status.has_ip) {
            lvl = blusys::badge_level::warning;
            txt = "WiFi: Connecting…";
        } else if (status.wifi_connected) {
            lvl = blusys::badge_level::success;
            txt = "WiFi: Connected";
        }
        blusys::status_badge_set_level(handles.wifi_badge, lvl);
        blusys::status_badge_set_text(handles.wifi_badge, txt);
    }

    if (handles.ip_label != nullptr && status.has_ip) {
        blusys::key_value_set_value(handles.ip_label, status.ip_info.ip);
    }

    if (handles.sntp_badge != nullptr) {
        blusys::status_badge_set_level(handles.sntp_badge,
            status.time_synced ? blusys::badge_level::success
                               : blusys::badge_level::warning);
    }

    if (handles.mdns_badge != nullptr) {
        blusys::status_badge_set_level(handles.mdns_badge,
            status.mdns_running ? blusys::badge_level::success
                                : blusys::badge_level::warning);
    }

    if (handles.ctrl_badge != nullptr) {
        blusys::status_badge_set_level(handles.ctrl_badge,
            status.local_ctrl_running ? blusys::badge_level::success
                                      : blusys::badge_level::warning);
    }
}

namespace {

blusys::app_ctx *g_reconnect_ctx = nullptr;

void on_reconnect_press(void * /*user_data*/)
{
    if (g_reconnect_ctx != nullptr) {
        g_reconnect_ctx->request_connectivity_reconnect();
    }
}

}  // namespace

lv_obj_t *connectivity_reconnect_button_create(lv_obj_t *parent, blusys::app_ctx &ctx,
                                                const char *label)
{
    g_reconnect_ctx = &ctx;

    blusys::button_config btn{};
    btn.label     = (label != nullptr) ? label : "Reconnect";
    btn.variant   = blusys::button_variant::secondary;
    btn.on_press  = on_reconnect_press;
    btn.user_data = nullptr;
    return blusys::button_create(parent, btn);
}

}  // namespace blusys::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
