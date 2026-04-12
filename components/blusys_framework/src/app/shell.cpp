#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/shell.hpp"
#include "blusys/app/view/screen_registry.hpp"
#include "blusys/app/app_ctx.hpp"
#include "blusys/framework/ui/primitives/col.hpp"
#include "blusys/framework/ui/primitives/label.hpp"
#include "blusys/framework/ui/primitives/row.hpp"
#include "blusys/framework/ui/primitives/screen.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/widgets/button/button.hpp"
#include "blusys/log.h"

static const char *TAG = "shell";

namespace blusys::app::view {

namespace {

// Module-level shell pointer, set during shell_load.
// Only one shell is active at a time (matches the blusys_ui singleton
// rule). Used by callbacks that need a stable reference to the shell.
static shell *g_active_shell = nullptr;

// Back button callback — navigates back through the shell's bound app_ctx.
void on_back_pressed(void * /*user_data*/)
{
    if (g_active_shell != nullptr && g_active_shell->ctx != nullptr) {
        g_active_shell->ctx->navigate_back();
    }
}

// Tab button callback — navigates to the tab's route.
struct tab_cb_data {
    std::size_t tab_index = 0;
};

static tab_cb_data g_tab_cb_data[kMaxTabs] = {};

void on_tab_pressed(void *user_data)
{
    auto *data = static_cast<tab_cb_data *>(user_data);
    if (data == nullptr || g_active_shell == nullptr || g_active_shell->ctx == nullptr) {
        return;
    }
    shell &s = *g_active_shell;
    if (data->tab_index < s.tab_count) {
        s.ctx->navigate_to(s.tabs[data->tab_index].route_id);
        shell_set_active_tab(s, data->tab_index);
    }
}

}  // namespace

shell shell_create(const shell_config &config)
{
    const auto &t = blusys::ui::theme();
    shell s{};

    // Root screen — non-scrollable, column layout.
    s.root = blusys::ui::screen_create({.scrollable = false});
    lv_obj_set_flex_flow(s.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s.root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(s.root, 0, 0);
    lv_obj_set_style_pad_gap(s.root, 0, 0);

    // Header bar.
    if (config.header.enabled) {
        const int h = (config.header.height >= 0) ? config.header.height
                                                   : (t.spacing_xl + t.spacing_sm);
        s.header = lv_obj_create(s.root);
        lv_obj_set_size(s.header, LV_PCT(100), h);
        lv_obj_set_flex_flow(s.header, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(s.header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(s.header, t.spacing_sm, 0);
        lv_obj_set_style_pad_ver(s.header, 0, 0);
        lv_obj_set_style_pad_gap(s.header, t.spacing_sm, 0);
        lv_obj_set_style_bg_color(s.header, t.color_surface, 0);
        lv_obj_set_style_bg_opa(s.header, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s.header, 1, 0);
        lv_obj_set_style_border_color(s.header, t.color_outline_variant, 0);
        lv_obj_set_style_border_side(s.header, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_remove_flag(s.header, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_grow(s.header, 0);

        // Back button (hidden by default).
        s.header_back = blusys::ui::button_create(s.header, {
            .label    = "<",
            .variant  = blusys::ui::button_variant::ghost,
            .on_press = on_back_pressed,
            .user_data = nullptr,  // bound later via shell_set_tabs or shell_load
        });
        lv_obj_add_flag(s.header_back, LV_OBJ_FLAG_HIDDEN);

        // Title label.
        s.header_title = blusys::ui::label_create(s.header, {
            .text = config.header.title != nullptr ? config.header.title : "",
            .font = t.font_title,
        });
    }

    // Status bar.
    if (config.status.enabled) {
        const int h = (config.status.height >= 0) ? config.status.height
                                                   : t.spacing_lg;
        s.status = lv_obj_create(s.root);
        lv_obj_set_size(s.status, LV_PCT(100), h);
        lv_obj_set_flex_flow(s.status, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(s.status, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(s.status, t.spacing_sm, 0);
        lv_obj_set_style_pad_ver(s.status, 0, 0);
        lv_obj_set_style_pad_gap(s.status, t.spacing_xs, 0);
        lv_obj_set_style_bg_color(s.status, t.color_background, 0);
        lv_obj_set_style_bg_opa(s.status, LV_OPA_COVER, 0);
        lv_obj_remove_flag(s.status, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_grow(s.status, 0);
    }

    // Content area — fills remaining space. Use a column flex container so the
    // scrollable page column is a flex item with a bounded height; otherwise the
    // scroll area can size to its content and push the tab bar off-screen.
    s.content_area = lv_obj_create(s.root);
    lv_obj_set_width(s.content_area, LV_PCT(100));
    lv_obj_set_flex_grow(s.content_area, 1);
    lv_obj_set_layout(s.content_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s.content_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s.content_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(s.content_area, 0, 0);
    lv_obj_set_style_bg_opa(s.content_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s.content_area, 0, 0);
    lv_obj_remove_flag(s.content_area, LV_OBJ_FLAG_SCROLLABLE);

    // Tab bar (items added via shell_set_tabs).
    if (config.tabs.enabled) {
        const int h = (config.tabs.height >= 0) ? config.tabs.height
                                                 : (t.spacing_xl + t.spacing_sm);
        s.tab_bar = lv_obj_create(s.root);
        lv_obj_set_size(s.tab_bar, LV_PCT(100), h);
        lv_obj_set_flex_flow(s.tab_bar, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(s.tab_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(s.tab_bar, 0, 0);
        lv_obj_set_style_pad_gap(s.tab_bar, 0, 0);
        lv_obj_set_style_bg_color(s.tab_bar, t.color_surface, 0);
        lv_obj_set_style_bg_opa(s.tab_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s.tab_bar, 1, 0);
        lv_obj_set_style_border_color(s.tab_bar, t.color_outline_variant, 0);
        lv_obj_set_style_border_side(s.tab_bar, LV_BORDER_SIDE_TOP, 0);
        lv_obj_remove_flag(s.tab_bar, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_grow(s.tab_bar, 0);
    }

    return s;
}

void shell_load(shell &s)
{
    if (s.root == nullptr) {
        BLUSYS_LOGW(TAG, "shell_load called on uninitialized shell");
        return;
    }
    if (g_active_shell != nullptr && g_active_shell != &s) {
        BLUSYS_LOGW(TAG, "replacing active shell — only one shell at a time is supported");
    }
    g_active_shell = &s;
    lv_screen_load(s.root);
}

lv_obj_t *shell_content(shell &s)
{
    return s.content_area;
}

void shell_set_title(shell &s, const char *title)
{
    if (s.header_title != nullptr && title != nullptr) {
        lv_label_set_text(s.header_title, title);
    }
}

void shell_set_back_visible(shell &s, bool visible)
{
    if (s.header_back == nullptr) {
        return;
    }
    if (visible) {
        lv_obj_remove_flag(s.header_back, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s.header_back, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *shell_status_surface(shell &s)
{
    return s.status;
}

void shell_set_tabs(shell &s, const shell_tab_item *items, std::size_t count,
                    app_ctx *ctx)
{
    if (s.tab_bar == nullptr || items == nullptr || count == 0) {
        return;
    }

    s.ctx = ctx;
    s.tab_count = (count > kMaxTabs) ? kMaxTabs : count;

    for (std::size_t i = 0; i < s.tab_count; ++i) {
        s.tabs[i] = items[i];

        g_tab_cb_data[i] = {.tab_index = i};

        blusys::ui::button_create(s.tab_bar, {
            .label     = items[i].label != nullptr ? items[i].label : "?",
            .variant   = blusys::ui::button_variant::ghost,
            .on_press  = on_tab_pressed,
            .user_data = &g_tab_cb_data[i],
        });
    }

    // Highlight the initial tab.
    shell_set_active_tab(s, 0);
}

void shell_set_active_tab(shell &s, std::size_t index)
{
    if (s.tab_bar == nullptr || index >= s.tab_count) {
        return;
    }

    const auto &t = blusys::ui::theme();
    s.active_tab = index;

    // Walk tab_bar children and set visual state.
    std::uint32_t child_count = lv_obj_get_child_count(s.tab_bar);
    for (std::uint32_t i = 0; i < child_count && i < s.tab_count; ++i) {
        lv_obj_t *child = lv_obj_get_child(s.tab_bar, static_cast<int32_t>(i));
        if (child == nullptr) {
            continue;
        }
        if (i == static_cast<std::uint32_t>(index)) {
            lv_obj_set_style_text_color(child, t.color_primary, 0);
            lv_obj_set_style_border_width(child, 2, LV_PART_MAIN);
            lv_obj_set_style_border_color(child, t.color_primary, LV_PART_MAIN);
            lv_obj_set_style_border_side(child, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_color(child, t.color_on_surface, 0);
            lv_obj_set_style_border_width(child, 0, LV_PART_MAIN);
        }
    }
}

void shell_sync_tabs_for_nav_stack(shell &s, const screen_registry &nav)
{
    if (s.tab_bar == nullptr || s.tab_count == 0) {
        return;
    }

    const std::size_t depth = nav.nav_stack_size();
    for (std::size_t ri = depth; ri > 0; --ri) {
        const std::uint32_t route = nav.nav_route_at(ri - 1);
        for (std::size_t ti = 0; ti < s.tab_count; ++ti) {
            if (s.tabs[ti].route_id == route) {
                shell_set_active_tab(s, ti);
                if (s.header_title != nullptr) {
                    const char *title = s.tabs[ti].header_title != nullptr ? s.tabs[ti].header_title
                                                                           : s.tabs[ti].label;
                    if (title != nullptr) {
                        shell_set_title(s, title);
                    }
                }
                return;
            }
        }
    }
}

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
