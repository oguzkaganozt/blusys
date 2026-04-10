#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/page.hpp"
#include "blusys/framework/ui/input/encoder.hpp"
#include "blusys/framework/ui/primitives/col.hpp"
#include "blusys/framework/ui/primitives/screen.hpp"
#include "blusys/framework/ui/theme.hpp"

namespace blusys::app::view {

page page_create(const page_config &config)
{
    lv_obj_t *screen = blusys::ui::screen_create({
        .scrollable = config.scrollable,
    });

    const int pad = (config.padding >= 0) ? config.padding : blusys::ui::theme().spacing_lg;
    const int gap = (config.gap >= 0) ? config.gap : blusys::ui::theme().spacing_md;

    lv_obj_t *content = blusys::ui::col_create(screen, {
        .gap     = gap,
        .padding = pad,
    });

    lv_group_t *group = blusys::ui::create_encoder_group();

    return {
        .screen  = screen,
        .content = content,
        .group   = group,
    };
}

void page_load(page &p)
{
    blusys::ui::auto_focus_screen(p.screen, p.group);
    lv_screen_load(p.screen);
}

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
