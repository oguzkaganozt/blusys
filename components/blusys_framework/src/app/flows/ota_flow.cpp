#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/flows/ota_flow.hpp"
#include "blusys/app/view/page.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/widgets/progress/progress.hpp"
#include "blusys/framework/ui/primitives/label.hpp"

#include <cstdio>

namespace blusys::app::flows {

namespace {

lv_obj_t *ota_screen_create_impl(const ota_flow_config *cfg,
                                  ota_screen_handles *handles,
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
    const char *title = (cfg != nullptr && cfg->title != nullptr) ? cfg->title : "Firmware Update";
    lv_obj_t *title_lbl = lv_label_create(page.content);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_font(title_lbl, t.font_title, 0);
    lv_obj_set_style_text_color(title_lbl, t.color_on_surface, 0);
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title_lbl, LV_PCT(100));

    // Progress bar.
    blusys::ui::progress_config pcfg{};
    pcfg.min      = 0;
    pcfg.max      = 100;
    pcfg.initial  = 0;
    pcfg.show_pct = true;
    lv_obj_t *progress = blusys::ui::progress_create(page.content, pcfg);
    lv_obj_set_width(progress, LV_PCT(80));

    // Status label.
    const char *checking = (cfg != nullptr && cfg->checking_msg != nullptr) ? cfg->checking_msg : "Checking...";
    lv_obj_t *status_lbl = lv_label_create(page.content);
    lv_label_set_text(status_lbl, checking);
    lv_obj_set_style_text_font(status_lbl, t.font_body, 0);
    lv_obj_set_style_text_color(status_lbl, t.color_on_surface, 0);
    lv_obj_set_style_text_opa(status_lbl, LV_OPA_70, 0);
    lv_obj_set_style_text_align(status_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(status_lbl, LV_PCT(100));

    if (handles != nullptr) {
        handles->progress_bar = progress;
        handles->status_label = status_lbl;
    }

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return page.screen;
}

}  // namespace

lv_obj_t *ota_screen_create(app_ctx & /*ctx*/, const void *params,
                             lv_group_t **group_out)
{
    return ota_screen_create_impl(
        static_cast<const ota_flow_config *>(params), nullptr, group_out);
}

lv_obj_t *ota_screen_create(app_ctx & /*ctx*/, const ota_flow_config &config,
                             ota_screen_handles &handles,
                             lv_group_t **group_out)
{
    return ota_screen_create_impl(&config, &handles, group_out);
}

void ota_screen_update(ota_screen_handles &handles,
                        const ota_status &status,
                        const ota_flow_config &config)
{
    if (handles.progress_bar != nullptr) {
        blusys::ui::progress_set_value(handles.progress_bar,
                                        static_cast<std::int32_t>(status.progress_pct));
    }

    if (handles.status_label != nullptr) {
        const char *msg = config.checking_msg;
        if (status.downloading) {
            msg = config.downloading_msg;
        } else if (status.apply_complete) {
            msg = config.success_msg;
        } else if (status.download_complete) {
            msg = config.applying_msg;
        }
        if (msg == nullptr) {
            msg = "";
        }
        lv_label_set_text(handles.status_label, msg);
    }
}

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
