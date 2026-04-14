// Gateway reference — UI layer (optional interactive variant).
//
// Provides an operator dashboard, status screen, settings, and about
// screen.  The same gateway logic runs headless without this layer.

#include "ui/app_ui.hpp"

#include "blusys/framework/flows/settings.hpp"
#include "blusys/framework/ui/screens/about.hpp"
#include "blusys/framework/ui/screens/status.hpp"
#include "blusys/framework/ui/binding/action_widgets.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/version.h"

namespace surface_surface_gateway::system {
const char *gateway_build_version_for_build();
const char *gateway_profile_label_for_build();
const char *gateway_hardware_label_for_build();
}

namespace surface_surface_gateway::ui {

namespace view = blusys::app::view;

using surface_surface_gateway::app_state;

namespace {

view::page make_page(blusys::app::app_ctx &ctx, bool scrollable)
{
    if (ctx.services().shell() != nullptr) {
        return view::page_create_in(ctx.services().shell()->content_area, {.scrollable = scrollable});
    }
    return view::page_create({.scrollable = scrollable});
}

// ---- handle clearing (on_hide user_data is app_ctx from register_screens) ----

void on_dashboard_hide(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr) {
        return;
    }
    auto *st = ctx->product_state<app_state>();
    if (st == nullptr) {
        return;
    }
    st->dash_devices    = nullptr;
    st->dash_throughput = nullptr;
    st->dash_uplink     = nullptr;
    st->dash_storage    = nullptr;
    st->dash_table      = nullptr;
}

void on_status_hide(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr) {
        return;
    }
    auto *st = ctx->product_state<app_state>();
    if (st != nullptr) {
        st->status_handles = {};
    }
}

// ---- on_show callbacks ----

void on_dashboard_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->services().shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->services().shell(), "Gateway");
    view::shell_set_active_tab(*ctx->services().shell(), 0);
}

void on_status_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->services().shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->services().shell(), "System Status");
    view::shell_set_active_tab(*ctx->services().shell(), 1);
}

void on_settings_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->services().shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->services().shell(), "Settings");
    view::shell_set_active_tab(*ctx->services().shell(), 2);
}

void on_about_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx != nullptr && ctx->services().shell() != nullptr) {
        view::shell_set_title(*ctx->services().shell(), "About");
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
    auto *st = ctx.product_state<app_state>();
    if (st == nullptr) {
        return nullptr;
    }

    auto page = make_page(ctx, true);

    auto *hero = view::card(page.content, "Operator Dashboard");
    lv_obj_set_width(hero, LV_PCT(100));

    st->dash_devices = view::key_value(hero, "Devices", "0 / 3");
    st->dash_throughput = view::key_value(hero, "Throughput", "0.0 msg/s");
    st->dash_uplink = view::key_value(hero, "Uplink", "Offline");
    st->dash_storage = view::key_value(hero, "Storage", "Pending");

    auto *actions = view::row(page.content);
    view::button(actions, "Status", action{.tag = action_tag::show_status}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(actions, "Settings", action{.tag = action_tag::show_settings}, &ctx,
                 blusys::ui::button_variant::ghost);

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return ctx.services().shell() != nullptr ? page.content : page.screen;
}

lv_obj_t *create_status(blusys::app::app_ctx &ctx, const void * /*params*/,
                        lv_group_t **group_out)
{
    auto *st = ctx.product_state<app_state>();
    if (st == nullptr) {
        return nullptr;
    }

    const blusys::app::screens::status_screen_config config{
        .show_connectivity = true,
        .show_diagnostics  = true,
        .show_storage      = false,
        .show_bluetooth    = false,
    };
    return blusys::app::screens::status_screen_create(
        ctx, config, st->status_handles, group_out);
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
            .id           = 1,
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
        {.key = "Interface", .value = "surface"},
        {.key = "Profile", .value = surface_gateway::system::gateway_profile_label_for_build()},
        {.key = "Identity", .value = "operational_light"},
    };

    const blusys::app::screens::about_screen_config config{
        .product_name     = "Blusys Gateway",
        .firmware_version = surface_gateway::system::gateway_build_version_for_build(),
        .hardware_version = surface_gateway::system::gateway_hardware_label_for_build(),
        .serial_number    = "GW-0001",
        .build_date       = __DATE__,
        .extras           = extras,
        .extra_count      = sizeof(extras) / sizeof(extras[0]),
    };

    return blusys::app::screens::about_screen_create(ctx, &config, group_out);
}

void register_all_screens(view::screen_router *router, blusys::app::app_ctx &ctx)
{
    static const view::screen_registration k_routes[] = {
        view::screen_row(route_dashboard, &create_dashboard, nullptr, &on_dashboard_show,
                         &on_dashboard_hide),
        view::screen_row(route_status, &create_status, nullptr, &on_status_show, &on_status_hide),
        view::screen_row(route_settings, &create_settings, nullptr, &on_settings_show, nullptr),
        view::screen_row(route_about, &create_about, nullptr, &on_about_show, nullptr),
    };

    router->register_screens(&ctx, k_routes, sizeof(k_routes) / sizeof(k_routes[0]));
}

}  // namespace

// ---- on_init ----

void on_init(blusys::app::app_ctx &ctx, blusys::app::app_services &svc, app_state &state)
{
    (void)svc;
    (void)state;

    if (ctx.services().shell() != nullptr) {
        const view::shell_tab_item tabs[] = {
            {.label = "Dash", .route_id = route_dashboard},
            {.label = "State", .route_id = route_status},
            {.label = "Setup", .route_id = route_settings},
        };
        view::shell_set_tabs(*ctx.services().shell(), tabs, sizeof(tabs) / sizeof(tabs[0]), &ctx.services());

        if (lv_obj_t *surface = view::shell_status_surface(*ctx.services().shell());
            surface != nullptr) {
            state.shell_badge = view::status_badge(
                surface, "Offline", blusys::ui::badge_level::warning);
            state.shell_detail = view::label(
                surface, "0 dev  0 msg/s", blusys::ui::theme().font_body_sm);
        }
    }

    auto *router = ctx.services().screen_router();
    if (router == nullptr) {
        return;
    }

    register_all_screens(router, ctx);

    ctx.services().navigate_to(route_dashboard);
}

}  // namespace surface_surface_gateway::ui
