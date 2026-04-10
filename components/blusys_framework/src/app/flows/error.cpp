#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/flows/error.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/widgets/button/button.hpp"

namespace blusys::app::flows {

lv_obj_t *error_panel_create(lv_obj_t *parent,
                              const error_display_config &config,
                              blusys::ui::press_cb_t on_retry,
                              void *retry_ctx,
                              blusys::ui::press_cb_t on_dismiss,
                              void *dismiss_ctx)
{
    const auto &t = blusys::ui::theme();

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(panel, t.spacing_lg, 0);
    lv_obj_set_style_pad_gap(panel, t.spacing_sm, 0);

    // Title.
    if (config.title != nullptr) {
        lv_obj_t *title = lv_label_create(panel);
        lv_label_set_text(title, config.title);
        lv_obj_set_style_text_font(title, t.font_title, 0);
        lv_obj_set_style_text_color(title, t.color_error, 0);
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(title, LV_PCT(100));
    }

    // Message.
    if (config.message != nullptr) {
        lv_obj_t *msg = lv_label_create(panel);
        lv_label_set_text(msg, config.message);
        lv_obj_set_style_text_font(msg, t.font_body, 0);
        lv_obj_set_style_text_color(msg, t.color_on_surface, 0);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(msg, LV_PCT(100));
    }

    // Detail.
    if (config.detail != nullptr) {
        lv_obj_t *detail = lv_label_create(panel);
        lv_label_set_text(detail, config.detail);
        lv_obj_set_style_text_font(detail, t.font_body_sm, 0);
        lv_obj_set_style_text_color(detail, t.color_on_surface, 0);
        lv_obj_set_style_text_opa(detail, LV_OPA_60, 0);
        lv_obj_set_style_text_align(detail, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(detail, LV_PCT(100));
    }

    // Button row.
    lv_obj_t *btn_row = lv_obj_create(panel);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_size(btn_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(btn_row, t.spacing_sm, 0);
    lv_obj_set_style_pad_top(btn_row, t.spacing_md, 0);

    // Retry button.
    if (config.retry_label != nullptr) {
        blusys::ui::button_config btn_cfg{};
        btn_cfg.label     = config.retry_label;
        btn_cfg.variant   = blusys::ui::button_variant::primary;
        btn_cfg.on_press  = on_retry;
        btn_cfg.user_data = retry_ctx;
        blusys::ui::button_create(btn_row, btn_cfg);
    }

    // Dismiss button.
    if (config.dismiss_label != nullptr) {
        blusys::ui::button_config btn_cfg{};
        btn_cfg.label     = config.dismiss_label;
        btn_cfg.variant   = blusys::ui::button_variant::ghost;
        btn_cfg.on_press  = on_dismiss;
        btn_cfg.user_data = dismiss_ctx;
        blusys::ui::button_create(btn_row, btn_cfg);
    }

    return panel;
}

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
