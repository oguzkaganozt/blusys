#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/screens/diagnostics_screen.hpp"
#include "blusys/app/view/page.hpp"
#include "blusys/app/view/shell.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/widgets/button/button.hpp"
#include "blusys/framework/ui/primitives/divider.hpp"

namespace blusys::app::screens {

lv_obj_t *diagnostics_screen_create(app_ctx &ctx, const void *params,
                                     lv_group_t **group_out)
{
    const auto *cfg = static_cast<const diagnostics_screen_config *>(params);
    const auto &t   = blusys::ui::theme();

    view::page page;
    const bool in_shell =
        ctx.services().shell() != nullptr && ctx.services().shell()->content_area != nullptr;
    if (in_shell) {
        page = view::page_create_in(ctx.services().shell()->content_area, {.scrollable = true});
    } else {
        page = view::page_create({.scrollable = true});
    }

    // Title.
    lv_obj_t *title = lv_label_create(page.content);
    lv_label_set_text(title, "Diagnostics");
    lv_obj_set_style_text_font(title, t.font_title, 0);
    lv_obj_set_style_text_color(title, t.color_on_surface, 0);
    lv_obj_set_width(title, LV_PCT(100));

    // Diagnostics panel.
    flows::diagnostics_panel_create(page.content);

    bool show_restart = (cfg == nullptr) || cfg->show_restart_button;

    if (show_restart) {
        blusys::ui::divider_create(page.content, {});

        blusys::ui::button_config btn_cfg{};
        btn_cfg.label     = "Restart Device";
        btn_cfg.variant   = blusys::ui::button_variant::danger;
        btn_cfg.on_press  = (cfg != nullptr) ? cfg->on_restart : nullptr;
        btn_cfg.user_data = (cfg != nullptr) ? cfg->restart_ctx : nullptr;
        lv_obj_t *btn = blusys::ui::button_create(page.content, btn_cfg);
        if (btn != nullptr) {
            lv_group_add_obj(page.group, btn);
        }
    }

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return in_shell ? page.content : page.screen;
}

}  // namespace blusys::app::screens

#endif  // BLUSYS_FRAMEWORK_HAS_UI
