#include "ui/app_ui.hpp"

#include "blusys/app/flows/settings.hpp"
#include "blusys/app/screens/about_screen.hpp"
#include "blusys/app/screens/status_screen.hpp"
#include "blusys/app/view/action_widgets.hpp"
#include "blusys/framework/ui/widgets/data_table/data_table.hpp"
#include "blusys/version.h"

namespace interactive_panel::system {
const char *panel_build_version_for_build();
const char *panel_profile_label_for_build();
const char *panel_hardware_label_for_build();
}

namespace interactive_panel::ui {

namespace view = blusys::app::view;

namespace {

app_state *g_state = nullptr;

view::page make_page(blusys::app::app_ctx &ctx, bool scrollable)
{
    if (ctx.shell() != nullptr) {
        return view::page_create_in(ctx.shell()->content_area, {.scrollable = scrollable});
    }
    return view::page_create({.scrollable = scrollable});
}

void clear_dashboard_handles()
{
    if (g_state == nullptr) {
        return;
    }
    g_state->dash_chart = nullptr;
    g_state->dash_load = nullptr;
    g_state->dash_temp = nullptr;
    g_state->dash_mode = nullptr;
    g_state->dash_table = nullptr;
    g_state->dash_series = -1;
}

void clear_status_handles()
{
    if (g_state != nullptr) {
        g_state->status_handles = {};
    }
}

void on_dashboard_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "Panel");
    view::shell_set_active_tab(*ctx->shell(), 0);
}

void on_status_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "Operational Status");
    view::shell_set_active_tab(*ctx->shell(), 1);
}

void on_settings_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "Panel Settings");
    view::shell_set_active_tab(*ctx->shell(), 2);
}

void on_about_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx != nullptr && ctx->shell() != nullptr) {
        view::shell_set_title(*ctx->shell(), "About");
    }
}

void on_settings_changed(void *user_ctx, std::uint32_t setting_id, std::int32_t value)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_ctx);
    if (ctx == nullptr) {
        return;
    }

    switch (setting_id) {
    case 1:
        ctx->dispatch(action{.tag = action_tag::set_mode, .value = value});
        break;
    case 2:
        ctx->dispatch(action{.tag = action_tag::open_about});
        break;
    default:
        break;
    }
}

lv_obj_t *create_dashboard(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    auto page = make_page(ctx, true);

    auto *hero = view::card(page.content, "Operations");
    lv_obj_set_width(hero, LV_PCT(100));
    g_state->dash_chart = view::chart(hero, blusys::ui::chart_type::line,
                                      static_cast<std::int32_t>(app_state::history_size), 0, 100);
    g_state->dash_series = blusys::ui::chart_add_series(g_state->dash_chart,
                                                        blusys::ui::theme().color_primary);
    g_state->dash_load = view::key_value(hero, "Load", "34%");
    g_state->dash_temp = view::key_value(hero, "Temp", "27 C");
    g_state->dash_mode = view::key_value(hero, "Mode", mode_name(g_state->mode_index));

    static const blusys::ui::table_column columns[] = {
        {.header = "Metric"},
        {.header = "Value"},
    };
    g_state->dash_table = blusys::ui::data_table_create(page.content, {
        .columns = columns,
        .col_count = 2,
        .row_count = 4,
    });
    blusys::ui::data_table_set_cell(g_state->dash_table, 0, 0, "Metric");
    blusys::ui::data_table_set_cell(g_state->dash_table, 0, 1, "Value");
    blusys::ui::data_table_set_cell(g_state->dash_table, 1, 0, "Queue");
    blusys::ui::data_table_set_cell(g_state->dash_table, 2, 0, "Heap");
    blusys::ui::data_table_set_cell(g_state->dash_table, 3, 0, "Storage");

    auto *actions = view::row(page.content);
    view::button(actions, "Status", action{.tag = action_tag::show_status}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(actions, "Settings", action{.tag = action_tag::show_settings}, &ctx,
                 blusys::ui::button_variant::ghost);

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return ctx.shell() != nullptr ? page.content : page.screen;
}

lv_obj_t *create_status(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    const blusys::app::screens::status_screen_config config{
        .show_connectivity = true,
        .show_diagnostics = true,
        .show_storage = true,
        .show_bluetooth = false,
    };
    return blusys::app::screens::status_screen_create(ctx, config, g_state->status_handles, group_out);
}

lv_obj_t *create_settings(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    static constexpr const char *kModeOptions[] = {"Calm", "Balanced", "Peak"};

    blusys::app::flows::setting_item items[] = {
        {
            .label = "Display Mode",
            .type = blusys::app::flows::setting_type::section_header,
        },
        {
            .label = "Presentation",
            .description = "Change dashboard emphasis.",
            .type = blusys::app::flows::setting_type::dropdown,
            .dropdown_options = kModeOptions,
            .dropdown_count = 3,
            .dropdown_initial = g_state->mode_index,
        },
        {
            .label = "About Panel",
            .description = "Inspect the archetype metadata.",
            .type = blusys::app::flows::setting_type::action_button,
            .button_label = "Open",
        },
    };

    const blusys::app::flows::settings_screen_config config{
        .title = "Panel Settings",
        .items = items,
        .item_count = sizeof(items) / sizeof(items[0]),
        .on_changed = on_settings_changed,
        .user_ctx = &ctx,
    };

    return blusys::app::flows::settings_screen_create(ctx, &config, group_out);
}

lv_obj_t *create_about(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    const blusys::app::screens::about_extra_field extras[] = {
        {.key = "Archetype", .value = "interactive panel"},
        {.key = "Profile", .value = interactive_panel::system::panel_profile_label_for_build()},
        {.key = "Identity", .value = "operational_light"},
        {.key = "Focus", .value = "status and diagnostics"},
    };

    const blusys::app::screens::about_screen_config config{
        .product_name = "Ops Panel",
        .firmware_version = interactive_panel::system::panel_build_version_for_build(),
        .hardware_version = interactive_panel::system::panel_hardware_label_for_build(),
        .serial_number = "PANEL-0001",
        .build_date = __DATE__,
        .extras = extras,
        .extra_count = sizeof(extras) / sizeof(extras[0]),
    };

    return blusys::app::screens::about_screen_create(ctx, &config, group_out);
}

}  // namespace

void on_init(blusys::app::app_ctx &ctx, app_state &state)
{
    g_state = &state;

    if (ctx.shell() != nullptr) {
        const view::shell_tab_item tabs[] = {
            {.label = "Dash", .route_id = route_dashboard},
            {.label = "State", .route_id = route_status},
            {.label = "Setup", .route_id = route_settings},
        };
        view::shell_set_tabs(*ctx.shell(), tabs, sizeof(tabs) / sizeof(tabs[0]), &ctx);

        if (lv_obj_t *surface = view::shell_status_surface(*ctx.shell()); surface != nullptr) {
            state.shell_badge = view::status_badge(surface, "Warmup", blusys::ui::badge_level::warning);
            state.shell_detail = view::label(surface, "Balanced  Q3  27C", blusys::ui::theme().font_body_sm);
        }
    }

    auto *router = ctx.screen_router();
    if (router == nullptr) {
        return;
    }

    router->register_screen(route_dashboard, &create_dashboard, nullptr,
                            {.on_show = on_dashboard_show, .on_hide = +[](lv_obj_t *, void *) { clear_dashboard_handles(); }, .user_data = &ctx});
    router->register_screen(route_status, &create_status, nullptr,
                            {.on_show = on_status_show, .on_hide = +[](lv_obj_t *, void *) { clear_status_handles(); }, .user_data = &ctx});
    router->register_screen(route_settings, &create_settings, nullptr,
                            {.on_show = on_settings_show, .user_data = &ctx});
    router->register_screen(route_about, &create_about, nullptr,
                            {.on_show = on_about_show, .user_data = &ctx});

    ctx.navigate_to(route_dashboard);
}

}  // namespace interactive_panel::ui
