#include "ui/app_ui.hpp"

#include "blusys/framework/ui/screens/about.hpp"
#include "blusys/framework/ui/binding/action_widgets.hpp"
#include "blusys/framework/ui/composition/page.hpp"
#include "blusys/framework/ui/composition/screen_router.hpp"

namespace mqtt_dashboard::system {
const char *mqtt_dashboard_version_for_build();
const char *mqtt_dashboard_profile_label_for_build();
const char *mqtt_dashboard_hardware_label_for_build();
}  // namespace mqtt_dashboard::system

namespace mqtt_dashboard::ui {
namespace {

namespace view = blusys::app::view;

using mqtt_dashboard::action;
using mqtt_dashboard::action_tag;
using mqtt_dashboard::app_state;
using mqtt_dashboard::route_about;
using mqtt_dashboard::route_live;

view::page make_page(blusys::app::app_ctx &ctx, bool scrollable)
{
    view::page_config cfg{.scrollable = scrollable};
    if (ctx.services().shell() != nullptr) {
        return view::page_create_in(ctx.services().shell()->content_area, cfg);
    }
    return view::page_create(cfg);
}

void on_live_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->services().shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->services().shell(), "MQTT live");
    view::shell_set_active_tab(*ctx->services().shell(), 0);
    // Recreated labels start at placeholders; re-sync from mqtt_host status.
    (void)ctx->dispatch(mqtt_dashboard::action{.tag = mqtt_dashboard::action_tag::mqtt_refresh});
}

void on_live_hide(lv_obj_t *screen, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr) {
        return;
    }
    auto *st = ctx->product_state<app_state>();
    if (st == nullptr) {
        return;
    }
    if (st->live_screen_root != nullptr && screen == st->live_screen_root) {
        st->status_line       = nullptr;
        st->last_rx_line      = nullptr;
        st->metrics_line      = nullptr;
        st->live_screen_root = nullptr;
    }
}

void on_about_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->services().shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->services().shell(), "About");
    view::shell_set_active_tab(*ctx->services().shell(), 1);
}

void on_about_hide(lv_obj_t * /*screen*/, void * /*user_data*/)
{
}

lv_obj_t *create_live(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    auto *st = ctx.product_state<app_state>();
    if (st == nullptr) {
        return nullptr;
    }

    auto page = make_page(ctx, true);

    auto *card = view::card(page.content, "Broker & traffic");
    lv_obj_set_width(card, LV_PCT(100));
    st->status_line  = view::label(card, "--");
    st->last_rx_line = view::label(card, "Last: --");
    st->metrics_line = view::label(card, "");

    auto *btn_row = view::row(page.content);
    lv_obj_set_width(btn_row, LV_PCT(100));
    view::button(btn_row, "Ping", action{.tag = action_tag::publish_ping}, &ctx,
                 blusys::ui::button_variant::primary);
    view::button(btn_row, "Toggle", action{.tag = action_tag::publish_toggle}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(btn_row, "Scene", action{.tag = action_tag::publish_scene}, &ctx,
                 blusys::ui::button_variant::secondary);

    st->live_screen_root = ctx.services().shell() != nullptr ? page.content : page.screen;

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return ctx.services().shell() != nullptr ? page.content : page.screen;
}

lv_obj_t *create_about(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    const blusys::app::screens::about_extra_field extras[] = {
        {.key = "Example", .value = "mqtt_dashboard"},
        {.key = "Profile", .value = mqtt_dashboard::system::mqtt_dashboard_profile_label_for_build()},
#if defined(ESP_PLATFORM)
        {.key = "Focus", .value = "MQTT on device"},
#else
        {.key = "Focus", .value = "MQTT / host SDL"},
#endif
    };

    const blusys::app::screens::about_screen_config config{
        .product_name     = "MQTT dashboard",
        .firmware_version = mqtt_dashboard::system::mqtt_dashboard_version_for_build(),
        .hardware_version = mqtt_dashboard::system::mqtt_dashboard_hardware_label_for_build(),
        .serial_number    = "MQTT-0001",
        .build_date       = __DATE__,
        .extras           = extras,
        .extra_count      = sizeof(extras) / sizeof(extras[0]),
    };

    return blusys::app::screens::about_screen_create(ctx, &config, group_out);
}

void register_all_screens(view::screen_router *router, blusys::app::app_ctx &ctx)
{
    static const view::screen_registration k_routes[] = {
        view::screen_row(route_live, &create_live, nullptr, &on_live_show, &on_live_hide),
        view::screen_row(route_about, &create_about, nullptr, &on_about_show, &on_about_hide),
    };

    router->register_screens(&ctx, k_routes, sizeof(k_routes) / sizeof(k_routes[0]));
}

}  // namespace

void on_init(blusys::app::app_ctx &ctx, blusys::app::app_services &svc, app_state &state)
{
    (void)svc;
    (void)state;

    if (ctx.services().shell() != nullptr) {
        const view::shell_tab_item tabs[] = {
            {.label = "Live", .route_id = route_live},
            {.label = "About", .route_id = route_about},
        };
        view::shell_set_tabs(*ctx.services().shell(), tabs, sizeof(tabs) / sizeof(tabs[0]), &ctx.services());

        if (lv_obj_t *surface = view::shell_status_surface(*ctx.services().shell()); surface != nullptr) {
            state.shell_badge =
                view::status_badge(surface, "Init", blusys::ui::badge_level::warning);
            state.shell_detail =
                view::label(surface, "MQTT demo", blusys::ui::theme().font_body_sm);
        }
    }

    auto *router = ctx.services().screen_router();
    if (router == nullptr) {
        return;
    }

    register_all_screens(router, ctx);
    ctx.services().navigate_to(route_live);
    (void)ctx.dispatch(mqtt_dashboard::action{.tag = mqtt_dashboard::action_tag::mqtt_refresh});
}

}  // namespace mqtt_dashboard::ui
