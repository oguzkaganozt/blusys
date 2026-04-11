#include "blusys/framework/ui/primitives/label.hpp"

#include "blusys/framework/ui/theme.hpp"

namespace blusys::ui {

lv_obj_t *label_create(lv_obj_t *parent, const label_config &config)
{
    lv_obj_t *label = lv_label_create(parent);
    const auto &t = theme();

    lv_label_set_text(label, config.text != nullptr ? config.text : "");
    lv_obj_set_style_text_color(label, t.color_on_surface, 0);
    const lv_font_t *font = config.font != nullptr ? config.font : t.font_body;
    lv_obj_set_style_text_font(label, font, 0);
    if (config.font == nullptr) {
        lv_obj_set_style_text_letter_space(label, t.text_letter_space_body, 0);
    }
    lv_obj_set_width(label, LV_SIZE_CONTENT);
    lv_obj_set_height(label, LV_SIZE_CONTENT);

    return label;
}

}  // namespace blusys::ui
