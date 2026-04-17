#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/screens/diagnostics.hpp"
#include "blusys/framework/ui/composition/page.hpp"
#include "blusys/framework/ui/composition/shell.hpp"
#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/framework/ui/widgets/button.hpp"
#include "blusys/framework/ui/primitives/divider.hpp"

namespace blusys::screens {

lv_obj_t *diagnostics_screen_create(app_ctx &ctx, const void *params,
                                     lv_group_t **group_out)
{
    const auto *cfg = static_cast<const diagnostics_screen_config *>(params);
    const auto &t   = blusys::theme();

    page page;
    auto *shell = ctx.fx().nav.shell();
    const bool in_shell = shell != nullptr && shell->content_area != nullptr;
    if (in_shell) {
        page = page_create_in(shell->content_area, {.scrollable = true});
    } else {
        page = page_create({.scrollable = true});
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
        blusys::divider_create(page.content, {});

        blusys::button_config btn_cfg{};
        btn_cfg.label     = "Restart Device";
        btn_cfg.variant   = blusys::button_variant::danger;
        btn_cfg.on_press  = (cfg != nullptr) ? cfg->on_restart : nullptr;
        btn_cfg.user_data = (cfg != nullptr) ? cfg->restart_ctx : nullptr;
        lv_obj_t *btn = blusys::button_create(page.content, btn_cfg);
        if (btn != nullptr) {
            lv_group_add_obj(page.group, btn);
        }
    }

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return in_shell ? page.content : page.screen;
}

}  // namespace blusys::screens

#endif  // BLUSYS_FRAMEWORK_HAS_UI
