#include "blusys/framework/ui/primitives/icon_label.hpp"

#include "blusys/framework/ui/style/icon_set.hpp"
#include "blusys/framework/ui/style/theme.hpp"

// Display-only icon + text row. No callbacks, no slot pool.
//
// Internal layout: a transparent row container with two lv_label children.
// Child 0 = icon glyph, child 1 = text. Products address them through
// the setters which look up children by index.

namespace blusys {
namespace {

constexpr int kChildIcon = 0;
constexpr int kChildText = 1;

}  // namespace

lv_obj_t *icon_label_create(lv_obj_t *parent, const icon_label_config &config)
{
    const auto &t = theme();

    // Row container.
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    int gap = config.gap >= 0 ? config.gap : t.spacing_sm;
    lv_obj_set_style_pad_column(row, gap, 0);
    lv_obj_set_width(row, LV_SIZE_CONTENT);
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Icon glyph.
    lv_obj_t *icon = lv_label_create(row);
    const lv_font_t *ifont = config.icon_font;
    if (ifont == nullptr) {
        ifont = (t.icons != nullptr && t.icons->font != nullptr) ? t.icons->font : t.font_body;
    }
    lv_obj_set_style_text_font(icon, ifont, 0);
    lv_color_t ic = config.use_default_color ? t.color_on_surface : config.icon_color;
    lv_obj_set_style_text_color(icon, ic, 0);
    lv_label_set_text(icon, config.icon != nullptr ? config.icon : "");

    // Text label.
    lv_obj_t *text = lv_label_create(row);
    const lv_font_t *tfont = config.text_font != nullptr ? config.text_font : t.font_body;
    lv_obj_set_style_text_font(text, tfont, 0);
    if (config.text_font == nullptr) {
        lv_obj_set_style_text_letter_space(text, t.text_letter_space_body, 0);
    }
    lv_obj_set_style_text_color(text, t.color_on_surface, 0);
    lv_label_set_text(text, config.text != nullptr ? config.text : "");

    return row;
}

void icon_label_set_text(lv_obj_t *icon_label, const char *text)
{
    if (icon_label == nullptr) {
        return;
    }
    lv_obj_t *lbl = lv_obj_get_child(icon_label, kChildText);
    if (lbl != nullptr) {
        lv_label_set_text(lbl, text != nullptr ? text : "");
    }
}

void icon_label_set_icon(lv_obj_t *icon_label, const char *icon)
{
    if (icon_label == nullptr) {
        return;
    }
    lv_obj_t *lbl = lv_obj_get_child(icon_label, kChildIcon);
    if (lbl != nullptr) {
        lv_label_set_text(lbl, icon != nullptr ? icon : "");
    }
}

}  // namespace blusys
