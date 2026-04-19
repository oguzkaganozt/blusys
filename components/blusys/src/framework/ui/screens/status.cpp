#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/screens/status.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/ui/composition/page.hpp"
#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/framework/ui/primitives/divider.hpp"
#include "blusys/framework/ui/widgets/card.hpp"

namespace blusys::screens {

namespace {

lv_obj_t *status_screen_create_impl(app_ctx &ctx,
                                    const status_screen_config *cfg,
                                    status_screen_handles *handles,
                                    lv_group_t **group_out)
{
    const auto &t = blusys::theme();

    page page;
    const bool in_shell = ctx.fx().nav.has_shell();
    if (in_shell) {
        page = page_create_in(ctx.fx().nav.content_area(), {.scrollable = true});
    } else {
        page = page_create({.scrollable = true});
    }

    // Title.
    lv_obj_t *title = lv_label_create(page.content);
    lv_label_set_text(title, "Status");
    lv_obj_set_style_text_font(title, t.font_title, 0);
    lv_obj_set_style_text_color(title, t.color_on_surface, 0);
    lv_obj_set_width(title, LV_PCT(100));

    bool show_conn = (cfg == nullptr) || cfg->show_connectivity;
    bool show_diag = (cfg == nullptr) || cfg->show_diagnostics;

    if (show_conn) {
        lv_obj_t *section_lbl = lv_label_create(page.content);
        lv_label_set_text(section_lbl, "Connectivity");
        lv_obj_set_style_text_font(section_lbl, t.font_label, 0);
        lv_obj_set_style_text_color(section_lbl, t.color_primary, 0);
        lv_obj_set_width(section_lbl, LV_PCT(100));

        flows::connectivity_panel_create(page.content,
            handles != nullptr ? &handles->connectivity : nullptr);
    }

    if (show_conn && show_diag) {
        blusys::divider_create(page.content, {});
    }

    if (show_diag) {
        lv_obj_t *section_lbl = lv_label_create(page.content);
        lv_label_set_text(section_lbl, "System");
        lv_obj_set_style_text_font(section_lbl, t.font_label, 0);
        lv_obj_set_style_text_color(section_lbl, t.color_primary, 0);
        lv_obj_set_width(section_lbl, LV_PCT(100));

        flows::diagnostics_panel_create(page.content,
            handles != nullptr ? &handles->diagnostics : nullptr);
        if (handles != nullptr) {
            if (const auto *diag = ctx.fx().diag.status(); diag != nullptr) {
                flows::diagnostics_panel_update(handles->diagnostics, *diag);
            }
        }
    }

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return in_shell ? page.content : page.screen;
}

}  // namespace

lv_obj_t *status_screen_create(app_ctx &ctx, const void *params,
                                lv_group_t **group_out)
{
    return status_screen_create_impl(
        ctx, static_cast<const status_screen_config *>(params), nullptr, group_out);
}

lv_obj_t *status_screen_create(app_ctx &ctx,
                                const status_screen_config &config,
                                status_screen_handles &handles,
                                lv_group_t **group_out)
{
    return status_screen_create_impl(ctx, &config, &handles, group_out);
}

void status_screen_update(status_screen_handles &handles, const app_ctx &ctx)
{
    const auto *conn = ctx.fx().connectivity.status();
    if (conn != nullptr) {
        flows::connectivity_panel_update(handles.connectivity, *conn);
    }

    const auto *diag = ctx.fx().diag.status();
    if (diag != nullptr) {
        flows::diagnostics_panel_update(handles.diagnostics, *diag);
    }
}

}  // namespace blusys::screens

#endif  // BLUSYS_FRAMEWORK_HAS_UI
