#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/flows/status.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/widgets/button/button.hpp"

namespace blusys::app::flows {

lv_obj_t *alert_create(lv_obj_t *parent, const alert_config &config)
{
    const auto &t = blusys::ui::theme();

    lv_obj_t *alert = lv_obj_create(parent);
    lv_obj_remove_style_all(alert);
    lv_obj_set_size(alert, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(alert, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(alert, t.radius_card, 0);
    lv_obj_set_style_pad_all(alert, t.spacing_md, 0);
    lv_obj_set_flex_flow(alert, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(alert, t.spacing_xs, 0);

    // Background color based on alert level.
    lv_color_t bg;
    switch (config.level) {
    case blusys::ui::badge_level::success: bg = t.color_success; break;
    case blusys::ui::badge_level::warning: bg = t.color_warning; break;
    case blusys::ui::badge_level::error:   bg = t.color_error;   break;
    default:                               bg = t.color_info;    break;
    }
    lv_obj_set_style_bg_color(alert, bg, 0);

    if (config.title != nullptr) {
        lv_obj_t *title = lv_label_create(alert);
        lv_label_set_text(title, config.title);
        lv_obj_set_style_text_font(title, t.font_label, 0);
        lv_obj_set_style_text_color(title, t.color_on_primary, 0);
    }

    if (config.message != nullptr) {
        lv_obj_t *msg = lv_label_create(alert);
        lv_label_set_text(msg, config.message);
        lv_obj_set_style_text_font(msg, t.font_body_sm, 0);
        lv_obj_set_style_text_color(msg, t.color_on_primary, 0);
    }

    // Auto-dismiss timer.
    if (config.auto_dismiss_ms > 0) {
        lv_timer_create(
            [](lv_timer_t *timer) {
                auto *obj = static_cast<lv_obj_t *>(lv_timer_get_user_data(timer));
                if (obj != nullptr) {
                    lv_obj_delete(obj);
                }
                lv_timer_delete(timer);
            },
            config.auto_dismiss_ms, alert);
    }

    return alert;
}

lv_obj_t *confirmation_create(lv_obj_t *parent,
                                const confirmation_config &config,
                                blusys::ui::press_cb_t on_confirm,
                                void *confirm_ctx,
                                blusys::ui::press_cb_t on_cancel,
                                void *cancel_ctx)
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

    if (config.title != nullptr) {
        lv_obj_t *title = lv_label_create(panel);
        lv_label_set_text(title, config.title);
        lv_obj_set_style_text_font(title, t.font_title, 0);
        lv_obj_set_style_text_color(title, t.color_on_surface, 0);
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(title, LV_PCT(100));
    }

    if (config.message != nullptr) {
        lv_obj_t *msg = lv_label_create(panel);
        lv_label_set_text(msg, config.message);
        lv_obj_set_style_text_font(msg, t.font_body, 0);
        lv_obj_set_style_text_color(msg, t.color_on_surface, 0);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(msg, LV_PCT(100));
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

    if (config.cancel_label != nullptr) {
        blusys::ui::button_config btn_cfg{};
        btn_cfg.label     = config.cancel_label;
        btn_cfg.variant   = blusys::ui::button_variant::ghost;
        btn_cfg.on_press  = on_cancel;
        btn_cfg.user_data = cancel_ctx;
        blusys::ui::button_create(btn_row, btn_cfg);
    }

    if (config.confirm_label != nullptr) {
        blusys::ui::button_config btn_cfg{};
        btn_cfg.label     = config.confirm_label;
        btn_cfg.variant   = blusys::ui::button_variant::primary;
        btn_cfg.on_press  = on_confirm;
        btn_cfg.user_data = confirm_ctx;
        blusys::ui::button_create(btn_row, btn_cfg);
    }

    return panel;
}

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
