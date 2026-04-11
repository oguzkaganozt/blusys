#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/flows/provisioning_flow.hpp"
#include "blusys/app/view/page.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/primitives/label.hpp"
#include "blusys/framework/ui/primitives/status_badge.hpp"

namespace blusys::app::flows {

namespace {

lv_obj_t *provisioning_screen_create_impl(const provisioning_flow_config *cfg,
                                           provisioning_screen_handles *handles,
                                           lv_group_t **group_out)
{
    const auto &t = blusys::ui::theme();

    auto page = view::page_create({.scrollable = false});

    lv_obj_set_flex_align(page.content,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(page.content, LV_PCT(100), LV_PCT(100));

    // Title.
    const char *title = (cfg != nullptr && cfg->title != nullptr) ? cfg->title : "Device Setup";
    lv_obj_t *title_lbl = lv_label_create(page.content);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_font(title_lbl, t.font_title, 0);
    lv_obj_set_style_text_color(title_lbl, t.color_on_surface, 0);
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title_lbl, LV_PCT(100));

    // QR payload label.
    const char *qr_label_text = (cfg != nullptr && cfg->qr_label != nullptr) ? cfg->qr_label : "Scan to connect:";
    lv_obj_t *qr_lbl = lv_label_create(page.content);
    lv_label_set_text(qr_lbl, qr_label_text);
    lv_obj_set_style_text_font(qr_lbl, t.font_body_sm, 0);
    lv_obj_set_style_text_color(qr_lbl, t.color_on_surface, 0);
    lv_obj_set_style_text_opa(qr_lbl, LV_OPA_70, 0);
    lv_obj_set_style_text_align(qr_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(qr_lbl, LV_PCT(100));
    lv_label_set_long_mode(qr_lbl, LV_LABEL_LONG_WRAP);

    // Status label.
    const char *waiting = (cfg != nullptr && cfg->waiting_msg != nullptr) ? cfg->waiting_msg : "Waiting...";
    lv_obj_t *status_lbl = lv_label_create(page.content);
    lv_label_set_text(status_lbl, waiting);
    lv_obj_set_style_text_font(status_lbl, t.font_body, 0);
    lv_obj_set_style_text_color(status_lbl, t.color_on_surface, 0);
    lv_obj_set_style_text_align(status_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(status_lbl, LV_PCT(100));

    if (handles != nullptr) {
        handles->qr_label     = qr_lbl;
        handles->status_label = status_lbl;
    }

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return page.screen;
}

}  // namespace

lv_obj_t *provisioning_screen_create(app_ctx & /*ctx*/, const void *params,
                                      lv_group_t **group_out)
{
    return provisioning_screen_create_impl(
        static_cast<const provisioning_flow_config *>(params), nullptr, group_out);
}

lv_obj_t *provisioning_screen_create(app_ctx & /*ctx*/,
                                      const provisioning_flow_config &config,
                                      provisioning_screen_handles &handles,
                                      lv_group_t **group_out)
{
    return provisioning_screen_create_impl(&config, &handles, group_out);
}

void provisioning_screen_update(provisioning_screen_handles &handles,
                                 const provisioning_status &status,
                                 const provisioning_flow_config &config)
{
    if (handles.qr_label != nullptr && status.qr_payload[0] != '\0') {
        lv_label_set_text(handles.qr_label, status.qr_payload);
    }

    if (handles.status_label != nullptr) {
        const char *msg = config.waiting_msg;
        if (status.provision_success) {
            msg = config.success_msg;
        } else if (status.provision_failed) {
            msg = config.error_msg;
        } else if (status.credentials_received) {
            msg = "Connecting...";
        }
        if (msg == nullptr) {
            msg = "";
        }
        lv_label_set_text(handles.status_label, msg);
    }
}

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
