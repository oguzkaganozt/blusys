#include "blusys/framework/ui/primitives/key_value.hpp"

#include "blusys/framework/ui/theme.hpp"

// Display-only key-value text pair. No callbacks, no slot pool.
//
// Internal layout: a horizontal row with two lv_label children.
// Child 0 = key (dimmed, font_label), child 1 = value (normal, font_body).
// The row uses space-between alignment to push value to the right.

namespace blusys::ui {
namespace {

constexpr int kChildKey   = 0;
constexpr int kChildValue = 1;

}  // namespace

lv_obj_t *key_value_create(lv_obj_t *parent, const key_value_config &config)
{
    const auto &t = theme();

    // Row container.
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Key label — dimmed.
    lv_obj_t *key = lv_label_create(row);
    lv_obj_set_style_text_font(key, t.font_label, 0);
    lv_obj_set_style_text_color(key, t.color_on_surface, 0);
    // Dimmed but readable on dark surfaces (opa_disabled is too faint for labels).
    lv_obj_set_style_text_opa(key, LV_OPA_70, 0);
    lv_label_set_text(key, config.key != nullptr ? config.key : "");

    // Value label — normal weight.
    lv_obj_t *value = lv_label_create(row);
    lv_obj_set_style_text_font(value, t.font_body, 0);
    lv_obj_set_style_text_color(value, t.color_on_surface, 0);
    lv_label_set_text(value, config.value != nullptr ? config.value : "");

    return row;
}

void key_value_set_key(lv_obj_t *kv, const char *key)
{
    if (kv == nullptr) {
        return;
    }
    lv_obj_t *label = lv_obj_get_child(kv, kChildKey);
    if (label != nullptr) {
        lv_label_set_text(label, key != nullptr ? key : "");
    }
}

void key_value_set_value(lv_obj_t *kv, const char *value)
{
    if (kv == nullptr) {
        return;
    }
    lv_obj_t *label = lv_obj_get_child(kv, kChildValue);
    if (label != nullptr) {
        lv_label_set_text(label, value != nullptr ? value : "");
    }
}

}  // namespace blusys::ui
