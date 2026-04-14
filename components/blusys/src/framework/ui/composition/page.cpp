#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/composition/page.hpp"
#include "blusys/framework/ui/input/encoder.hpp"
#include "blusys/framework/ui/primitives/col.hpp"
#include "blusys/framework/ui/primitives/screen.hpp"
#include "blusys/framework/ui/style/theme.hpp"

namespace blusys {

page page_create(const page_config &config)
{
    lv_obj_t *screen = blusys::screen_create({
        .scrollable = config.scrollable,
    });

    const int pad = (config.padding >= 0) ? config.padding : blusys::theme().spacing_lg;
    const int gap = (config.gap >= 0) ? config.gap : blusys::theme().spacing_md;

    lv_obj_t *content = blusys::col_create(screen, {
        .gap     = gap,
        .padding = pad,
    });

    lv_group_t *group = blusys::create_encoder_group();

    return {
        .screen  = screen,
        .content = content,
        .group   = group,
    };
}

page page_create_in(lv_obj_t *parent, const page_config &config)
{
    const int pad = (config.padding >= 0) ? config.padding : blusys::theme().spacing_lg;
    const int gap = (config.gap >= 0) ? config.gap : blusys::theme().spacing_md;

    lv_obj_t *content = blusys::col_create(parent, {
        .gap     = gap,
        .padding = pad,
    });
    lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_min_height(content, 0, 0);

    if (config.scrollable) {
        lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_group_t *group = blusys::create_encoder_group();

    return {
        .screen  = parent,
        .content = content,
        .group   = group,
    };
}

void page_load(page &p)
{
    blusys::auto_focus_screen(p.screen, p.group);
    lv_screen_load(p.screen);

    // Attach the focus group to all registered encoder indevs.
    // Works on both host (keyboard encoder) and device (hardware encoder).
    lv_indev_t *indev = lv_indev_get_next(nullptr);
    while (indev != nullptr) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
            lv_indev_set_group(indev, p.group);
        }
        indev = lv_indev_get_next(indev);
    }
}

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
