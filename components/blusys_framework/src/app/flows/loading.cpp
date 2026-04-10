#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/flows/loading.hpp"
#include "blusys/framework/ui/theme.hpp"

namespace blusys::app::flows {

lv_obj_t *loading_create(lv_obj_t *parent, const loading_config &config)
{
    const auto &t = blusys::ui::theme();

    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(container, t.spacing_md, 0);

    if (config.show_spinner) {
        lv_obj_t *spinner = lv_spinner_create(container);
        lv_spinner_set_anim_params(spinner, 1000, 270);
        lv_obj_set_size(spinner, 32, 32);
        lv_obj_set_style_arc_color(spinner, t.color_primary, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(spinner, t.color_outline, LV_PART_MAIN);
        lv_obj_set_style_arc_width(spinner, 3, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(spinner, 3, LV_PART_MAIN);
    }

    if (config.message != nullptr) {
        lv_obj_t *label = lv_label_create(container);
        lv_label_set_text(label, config.message);
        lv_obj_set_style_text_font(label, t.font_body, 0);
        lv_obj_set_style_text_color(label, t.color_on_surface, 0);
        lv_obj_set_style_text_opa(label, LV_OPA_70, 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    }

    return container;
}

lv_obj_t *empty_state_create(lv_obj_t *parent, const empty_state_config &config)
{
    const auto &t = blusys::ui::theme();

    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(container, t.spacing_sm, 0);

    if (config.title != nullptr) {
        lv_obj_t *title = lv_label_create(container);
        lv_label_set_text(title, config.title);
        lv_obj_set_style_text_font(title, t.font_title, 0);
        lv_obj_set_style_text_color(title, t.color_on_surface, 0);
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    }

    if (config.message != nullptr) {
        lv_obj_t *msg = lv_label_create(container);
        lv_label_set_text(msg, config.message);
        lv_obj_set_style_text_font(msg, t.font_body_sm, 0);
        lv_obj_set_style_text_color(msg, t.color_on_surface, 0);
        lv_obj_set_style_text_opa(msg, LV_OPA_60, 0);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    }

    return container;
}

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
