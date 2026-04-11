#include "blusys/framework/ui/widgets/card/card.hpp"

#include "blusys/framework/ui/theme.hpp"

// Card container with optional title. No callbacks, no slot pool.
//
// Internal layout: a rounded container with surface background, shadow,
// and vertical flex. If a title is provided, it is child 0 (an lv_label);
// otherwise products add children directly.

namespace blusys::ui {
namespace {

lv_obj_t *title_from(lv_obj_t *card)
{
    // Title label is tagged via user_data.
    return static_cast<lv_obj_t *>(lv_obj_get_user_data(card));
}

}  // namespace

lv_obj_t *card_create(lv_obj_t *parent, const card_config &config)
{
    const auto &t = theme();

    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    int pad = config.padding >= 0 ? config.padding : t.spacing_md;
    lv_obj_set_style_pad_all(card, pad, 0);
    lv_obj_set_style_pad_row(card, t.spacing_sm, 0);

    // Elevated surface (card above page background).
    lv_obj_set_style_bg_color(card, t.color_surface_elevated, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, t.radius_card, 0);
    lv_obj_set_style_border_width(card, 0, 0);

    // Shadow / elevation.
    lv_obj_set_style_shadow_width(card, t.shadow_card_width, 0);
    lv_obj_set_style_shadow_ofs_y(card, t.shadow_card_ofs_y, 0);
    lv_obj_set_style_shadow_spread(card, t.shadow_card_spread, 0);
    lv_obj_set_style_shadow_color(card, t.shadow_color, 0);

    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Optional title.
    if (config.title != nullptr) {
        lv_obj_t *title = lv_label_create(card);
        lv_obj_set_style_text_font(title, t.font_title, 0);
        lv_obj_set_style_text_color(title, t.color_on_surface, 0);
        lv_label_set_text(title, config.title);
        lv_obj_set_user_data(card, title);
    }

    return card;
}

void card_set_title(lv_obj_t *card, const char *title)
{
    if (card == nullptr) {
        return;
    }
    lv_obj_t *label = title_from(card);
    if (label != nullptr && title != nullptr) {
        lv_label_set_text(label, title);
    }
}

}  // namespace blusys::ui
