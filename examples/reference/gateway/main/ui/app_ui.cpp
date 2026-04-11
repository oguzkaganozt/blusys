// Gateway reference — UI layer (optional interactive variant).
//
// Provides an operator dashboard, status screen, settings, and about
// screen.  The same gateway logic runs headless without this layer.

#include "ui/app_ui.hpp"

#include "blusys/app/flows/settings.hpp"
#include "blusys/app/screens/about_screen.hpp"
#include "blusys/app/screens/status_screen.hpp"
#include "blusys/app/view/action_widgets.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/version.h"

namespace gateway::system {
const char *gateway_build_version_for_build();
const char *gateway_profile_label_for_build();
const char *gateway_hardware_label_for_build();
}

namespace gateway::ui {

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

// ---- handle clearing ----

void clear_dashboard_handles()
{
    if (g_state == nullptr) {
        return;
    }
    g_state->dash_devices    = nullptr;
    g_state->dash_throughput = nullptr;
    g_state->dash_uplink     = nullptr;
    g_state->dash_storage    = nullptr;
    g_state->dash_table      = nullptr;
}

void clear_status_handles()
{
    if (g_state != nullptr) {
        g_state->status_handles = {};
    }
}

// ---- on_show callbacks ----

void on_dashboard_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "Gateway");
    view::shell_set_active_tab(*ctx->shell(), 0);
}

void on_status_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "System Status");
    view::shell_set_active_tab(*ctx->shell(), 1);
}

void on_settings_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "Settings");
    view::shell_set_active_tab(*ctx->shell(), 2);
}

void on_about_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx != nullptr && ctx->shell() != nullptr) {
        view::shell_set_title(*ctx->shell(), "About");
    }
}

// ---- settings callback ----

void on_settings_changed(void *user_ctx, std::uint32_t setting_id, std::int32_t /*value*/)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_ctx);
    if (ctx == nullptr) {
        return;
    }

    switch (setting_id) {
    case 1:
        ctx->dispatch(action{.tag = action_tag::open_about});
        break;
    default:
        break;
    }
}

// ---- screen factories ----

lv_obj_t *create_dashboard(blusys::app::app_ctx &ctx, const void * /*params*/,
                           lv_group_t **group_out)
{
    auto page = make_page(ctx, true);

    auto *hero = view::card(page.content, "Operator Dashboard");
    lv_obj_set_width(hero, LV_PCT(100));

    g_state->dash_devices = view::key_value(hero, "Devices", "0 / 3");
    g_state->dash_throughput = view::key_value(hero, "Throughput", "0.0 msg/s");
    g_state->dash_uplink = view::key_value(hero, "Uplink", "Offline");
    g_state->dash_storage = view::key_value(hero, "Storage", "Pending");

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

lv_obj_t *create_status(blusys::app::app_ctx &ctx, const void * /*params*/,
                        lv_group_t **group_out)
{
    const blusys::app::screens::status_screen_config config{
        .show_connectivity = true,
        .show_diagnostics  = true,
        .show_storage      = false,
        .show_bluetooth    = false,
    };
    return blusys::app::screens::status_screen_create(
        ctx, config, g_state->status_handles, group_out);
}

lv_obj_t *create_settings(blusys::app::app_ctx &ctx, const void * /*params*/,
                          lv_group_t **group_out)
{
    blusys::app::flows::setting_item items[] = {
        {
            .label = "Gateway",
            .type  = blusys::app::flows::setting_type::section_header,
        },
        {
            .label       = "About Device",
            .description = "Inspect gateway build and hardware details.",
            .type         = blusys::app::flows::setting_type::action_button,
            .button_label = "Open",
        },
    };

    const blusys::app::flows::settings_screen_config config{
        .title      = "Gateway Settings",
        .items      = items,
        .item_count = sizeof(items) / sizeof(items[0]),
        .on_changed = on_settings_changed,
        .user_ctx   = &ctx,
    };

    return blusys::app::flows::settings_screen_create(ctx, &config, group_out);
}

lv_obj_t *create_about(blusys::app::app_ctx &ctx, const void * /*params*/,
                       lv_group_t **group_out)
{
    const blusys::app::screens::about_extra_field extras[] = {
        {.key = "Archetype", .value = "gateway/controller"},
        {.key = "Profile", .value = gateway::system::gateway_profile_label_for_build()},
        {.key = "Identity", .value = "operational_light"},
    };

    const blusys::app::screens::about_screen_config config{
        .product_name     = "Blusys Gateway",
        .firmware_version = gateway::system::gateway_build_version_for_build(),
        .hardware_version = gateway::system::gateway_hardware_label_for_build(),
        .serial_number    = "GW-0001",
        .build_date       = __DATE__,
        .extras           = extras,
        .extra_count      = sizeof(extras) / sizeof(extras[0]),
    };

    return blusys::app::screens::about_screen_create(ctx, &config, group_out);
}

}  // namespace

// ---- on_init ----

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

        if (lv_obj_t *surface = view::shell_status_surface(*ctx.shell());
            surface != nullptr) {
            state.shell_badge = view::status_badge(
                surface, "Offline", blusys::ui::badge_level::warning);
            state.shell_detail = view::label(
                surface, "0 dev  0 msg/s", blusys::ui::theme().font_body_sm);
        }
    }

    auto *router = ctx.screen_router();
    if (router == nullptr) {
        return;
    }

    router->register_screen(route_dashboard, &create_dashboard, nullptr,
        {.on_show = on_dashboard_show,
         .on_hide = +[](lv_obj_t *, void *) { clear_dashboard_handles(); },
         .user_data = &ctx});
    router->register_screen(route_status, &create_status, nullptr,
        {.on_show = on_status_show,
         .on_hide = +[](lv_obj_t *, void *) { clear_status_handles(); },
         .user_data = &ctx});
    router->register_screen(route_settings, &create_settings, nullptr,
        {.on_show = on_settings_show,
         .user_data = &ctx});
    router->register_screen(route_about, &create_about, nullptr,
        {.on_show = on_about_show,
         .user_data = &ctx});

    ctx.navigate_to(route_dashboard);
}

}  // namespace gateway::ui
